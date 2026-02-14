#include "../../includes/handlers/CGIHandler.hpp"
#include "../../includes/handlers/ErrorHandler.hpp"
#include "../../includes/handlers/getType.hpp"
#include <fstream>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <limits.h>
#include <iostream>
#include <poll.h>



std::string CGIHandler::getFileExt(const std::string &scriptPath, const LocationConfig &config)
{
	size_t dotPos = scriptPath.rfind('.');
	if (dotPos == std::string::npos)
		return "";
	std::string ext = scriptPath.substr(dotPos);
	if (config.cgi_ext.find(ext) == config.cgi_ext.end())
		return "";
	if (!config.cgi_path.empty())
		return config.cgi_path;
	if (ext == ".py" || ext == ".py3")
		return "/usr/bin/python3";
	else if (ext == ".php")
		return "/usr/bin/php";
    else if (ext == ".sh")
        return "/usr/bin/bash";
	else if (ext == ".pl")
		return "/usr/bin/perl";
	else if (ext == ".rb")
		return "/usr/bin/ruby";
	return "";
}

bool CGIHandler::isExecutable(const std::string &path)
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return false;
	return access(path.c_str(), X_OK) == 0;
}

std::vector<std::string> CGIHandler::buildEnvVars(const Request &req, const std::string &scriptPath, const LocationConfig &config)
{
	(void)config;
	std::vector<std::string> envVars;

	envVars.push_back("REQUEST_METHOD=" + req.method);
	std::string scriptName = req.path;
    std::string pathInfo = req.path;
    std::string queryString = "";
    size_t queryPos = req.path.find('?');
    if (queryPos != std::string::npos)
    {
        scriptName = req.path.substr(0, queryPos);
        pathInfo = scriptName;
        queryString = req.path.substr(queryPos + 1);
    }
    envVars.push_back("SCRIPT_NAME=" + scriptName);
    envVars.push_back("PATH_INFO=" + pathInfo);
    envVars.push_back("QUERY_STRING=" + queryString);
	std::map<std::string, std::string>::const_iterator contentTypeIt = req.headers.find("content-type");
	if (contentTypeIt != req.headers.end())
		envVars.push_back("CONTENT_TYPE=" + contentTypeIt->second);
	else
		envVars.push_back("CONTENT_TYPE=");

	std::ostringstream contentLength;
	contentLength << req.body.size();
	envVars.push_back("CONTENT_LENGTH=" + contentLength.str());

	envVars.push_back("SERVER_NAME=localhost");
	envVars.push_back("SERVER_PORT=8080");
	envVars.push_back("SERVER_PROTOCOL=HTTP/1.0");
	envVars.push_back("SERVER_SOFTWARE=webserv/1.0");
	envVars.push_back("REQUEST_URI=" + req.path);
	envVars.push_back("GATEWAY_INTERFACE=CGI/1.1");
	envVars.push_back("REMOTE_ADDR=127.0.0.1");
	envVars.push_back("REMOTE_HOST=localhost");
	char resolvedPath[4096];
	if (realpath(scriptPath.c_str(), resolvedPath) != NULL)
		envVars.push_back("SCRIPT_FILENAME=" + std::string(resolvedPath));
	else
		envVars.push_back("SCRIPT_FILENAME=" + scriptPath);
	for (std::map<std::string, std::string>::const_iterator it = req.headers.begin();
			it != req.headers.end(); ++it)
	{
		std::string headerName = it->first;
		for (size_t i = 0; i < headerName.size(); ++i)
		{
			if (headerName[i] == '-')
				headerName[i] = '_';
			else if (headerName[i] >= 'a' && headerName[i] <= 'z')
				headerName[i] = headerName[i] - 'a' + 'A';
		}
		envVars.push_back("HTTP_" + headerName + "=" + it->second);
	}
	return envVars;
}

char** CGIHandler::envToArray(const std::vector<std::string> &envVars)
{
	char **envArray = new char*[envVars.size() + 1];
	for (size_t i = 0; i < envVars.size(); ++i)
	{
		envArray[i] = new char[envVars[i].size() + 1];
		std::strcpy(envArray[i], envVars[i].c_str());
	}
	envArray[envVars.size()] = NULL;
	return envArray;
}

void CGIHandler::freeEnvArray(char **envArray)
{
	if (!envArray)
		return;
	for (size_t i = 0; envArray[i] != NULL; ++i)
		delete[] envArray[i];
	delete[] envArray;
}

std::string readWithTimeout(int fd, pid_t pid, int timeoutSeconds)
{
    std::string output;
    char buffer[65536];

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;

    int timeoutMs = timeoutSeconds * 1000;

    while (true)
    {
        int ret = poll(&pfd, 1, timeoutMs);
        if (ret == 0)
        {
            kill(pid, SIGKILL);
            throw std::runtime_error("CGI Timeout");
        }
        else if (ret < 0)
        {
            throw std::runtime_error("Poll error");
        }

        ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
        if (bytesRead > 0)
            output.append(buffer, bytesRead);
        else if (bytesRead == 0)
            break;
        else if (errno != EAGAIN && errno != EINTR)
            break;
    }

    return output;
}

void closePipes(int inPipe[2], int outPipe[2])
{
    close(inPipe[0]);
	close(inPipe[1]);
	close(outPipe[0]);
	close(outPipe[1]);
}

Response buildCgiRes(size_t headerEnd, std::string cgiOutput, const std::string &scriptPath)
{
    Response res;
		res.status = 200;
		res.reason = "OK";

		if (headerEnd != std::string::npos)
		{
			std::string headers = cgiOutput.substr(0, headerEnd);
			size_t bodyStart = headerEnd + (cgiOutput.find("\r\n\r\n") != std::string::npos ? 4 : 2);
			res.body = cgiOutput.substr(bodyStart);
			std::istringstream headerStream(headers);
			std::string headerLine;
			while (std::getline(headerStream, headerLine))
			{
				size_t colonPos = headerLine.find(':');
				if (colonPos != std::string::npos)
				{
					std::string key = headerLine.substr(0, colonPos);
					std::string value = headerLine.substr(colonPos + 1);
					while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
						value = value.substr(1);
					while (!value.empty() && (value[value.size() - 1] == '\r' || value[value.size() - 1] == '\n'))
						value = value.substr(0, value.size() - 1);
					if (key == "Status")
					{
						size_t spacePos = value.find(' ');
						if (spacePos != std::string::npos)
						{
							std::istringstream statusStream(value.substr(0, spacePos));
							statusStream >> res.status;
							res.reason = value.substr(spacePos + 1);
						}
					}
					else
						res.headers[key] = value;
				}
			}
		}
		else
		{
			res.body = cgiOutput;
			res.headers["Content-Type"] = "text/html";
		}
		if (res.headers.find("Content-Type") == res.headers.end())
			res.headers["Content-Type"] = getType::getMimeType(scriptPath);
		std::stringstream len;
		len << res.body.size();
		res.headers["Content-Length"] = len.str();
		return res;
}


Response CGIHandler::handleCGIReq(const Request &req, const LocationConfig &config, const std::string &scriptPath)
{
	int inPipe[2];
	int outPipe[2];

    struct stat st;
	if (stat(scriptPath.c_str(), &st) != 0)
		return ErrorHandler::handleError(404, "Not Found", config);
	if (!S_ISREG(st.st_mode))
		return ErrorHandler::handleError(403, "Forbidden", config);
	
	// Check if file is readable
	if (access(scriptPath.c_str(), R_OK) != 0)
		return ErrorHandler::handleError(403, "Forbidden", config);

	// CGI scripts must have execute permission
	if (!isExecutable(scriptPath))
		return ErrorHandler::handleError(403, "Forbidden", config);

	std::string exten = getFileExt(scriptPath, config);
	if (exten.empty())
		return ErrorHandler::handleError(500, "Internal Server Error", config);
	
	if (stat(exten.c_str(), &st) != 0)
		return ErrorHandler::handleError(500, "Internal Server Error", config);

	std::vector<std::string> envVars = buildEnvVars(req, scriptPath, config);
	char **envArray = envToArray(envVars);

	if (pipe(inPipe) == -1 || pipe(outPipe) == -1)
	{
		freeEnvArray(envArray);
		return ErrorHandler::handleError(500, "Internal Server Error", config);
	}
	pid_t pid = fork();
	if (pid == -1)
	{
		closePipes(inPipe, outPipe);
		freeEnvArray(envArray);
		return ErrorHandler::handleError(500, "Internal Server Error", config);
	}
	if (pid == 0)
	{
		dup2(inPipe[0], STDIN_FILENO);
		dup2(outPipe[1], STDOUT_FILENO);
		dup2(outPipe[1], STDERR_FILENO);

		closePipes(inPipe, outPipe);
		char resolvedScriptPath[PATH_MAX];
		if (realpath(scriptPath.c_str(), resolvedScriptPath) == NULL)
			_exit(127);
		std::string absScriptPath(resolvedScriptPath);
		size_t lastSlash = absScriptPath.find_last_of('/');
		if (lastSlash != std::string::npos)
		{
			std::string scriptDir = absScriptPath.substr(0, lastSlash);
			chdir(scriptDir.c_str());
		}
		char *argv[3];
		if (exten.empty())
		{
			argv[0] = const_cast<char*>(absScriptPath.c_str());
			argv[1] = NULL;
			execve(absScriptPath.c_str(), argv, envArray);
		}
		else
		{
			argv[0] = const_cast<char*>(exten.c_str());
			argv[1] = const_cast<char*>(absScriptPath.c_str());
			argv[2] = NULL;
			execve(exten.c_str(), argv, envArray);
		}
		_exit(127);
	}
	else
	{
        int status;
		close(inPipe[0]);
		close(outPipe[1]);

		size_t total = 0;
        while (total < req.body.size())
        {
            ssize_t written = write(inPipe[1],
                req.body.c_str() + total,
                req.body.size() - total);

            if (written <= 0)
                break;

            total += written;
        }
		close(inPipe[1]);
        std::string cgiOutput;
        try
        {
            cgiOutput = readWithTimeout(outPipe[0],pid, 30);
        }
        catch (...)
        {
            close(outPipe[0]);
		    waitpid(pid, &status, 0);
    		freeEnvArray(envArray);
            return ErrorHandler::handleError(504, "Gateway Timeout", config);
        }
		close(outPipe[0]);
		waitpid(pid, &status, 0);
		freeEnvArray(envArray);
		if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
			return ErrorHandler::handleError(500, "Internal Server Error", config);
		if (WIFSIGNALED(status))
			return ErrorHandler::handleError(504, "Gateway Timeout", config);
		size_t headerEnd = cgiOutput.find("\r\n\r\n");
		if (headerEnd == std::string::npos)
			headerEnd = cgiOutput.find("\n\n");
		Response res = buildCgiRes(headerEnd, cgiOutput, scriptPath);
        return res;
	}
}
