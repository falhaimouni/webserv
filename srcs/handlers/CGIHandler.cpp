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
	
	// Check if we have a configured interpreter for this extension
	std::map<std::string, std::string>::const_iterator it = config.cgi_interpreters.find(ext);
	if (it != config.cgi_interpreters.end())
		return it->second;
	
	// Fall back to cgi_path if set
	if (!config.cgi_path.empty())
		return config.cgi_path;
	
	// Fall back to default interpreters
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

bool CGIHandler::isExecutable(const std::string &path, bool useInterpreter)
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return false;
	
	// If using an interpreter (Python, Bash, etc.), only need read permission
	if (useInterpreter)
		return access(path.c_str(), R_OK) == 0;
	
	// If direct execution, need execute permission
	return access(path.c_str(), X_OK) == 0;
}

std::vector<std::string> CGIHandler::buildEnvVars(const Request &req, const std::string &scriptPath, const LocationConfig &config)
{
	(void)config;
	std::vector<std::string> envVars;

	envVars.push_back("REQUEST_METHOD=" + req.method);
	
	// Extract SCRIPT_NAME and PATH_INFO from request path
	std::string pathWithoutQuery = req.path;
	std::string queryString = "";
	size_t queryPos = req.path.find('?');
	if (queryPos != std::string::npos)
	{
		pathWithoutQuery = req.path.substr(0, queryPos);
		queryString = req.path.substr(queryPos + 1);
	}
	
	// SCRIPT_NAME is the path to the script file
	std::string scriptName = pathWithoutQuery;
	std::string pathInfo = "";
	
	// Extract the script filename from the real path
	std::string realPathStr = scriptPath;
	size_t lastSlash = realPathStr.find_last_of('/');
	std::string scriptFileName;
	if (lastSlash != std::string::npos)
		scriptFileName = realPathStr.substr(lastSlash);
	
	// Find where the script name ends in the request path
	if (!scriptFileName.empty())
	{
		size_t scriptPos = pathWithoutQuery.find(scriptFileName);
		if (scriptPos != std::string::npos)
		{
			scriptName = pathWithoutQuery.substr(0, scriptPos + scriptFileName.length());
			if (scriptPos + scriptFileName.length() < pathWithoutQuery.length())
				pathInfo = pathWithoutQuery.substr(scriptPos + scriptFileName.length());
		}
	}
	
    envVars.push_back("SCRIPT_NAME=" + scriptName);
    envVars.push_back("PATH_INFO=" + pathInfo);
    envVars.push_back("QUERY_STRING=" + queryString);
	
	std::map<std::string, std::string>::const_iterator contentTypeIt = req.headers.find("content-type");
	if (contentTypeIt != req.headers.end())
		envVars.push_back("CONTENT_TYPE=" + contentTypeIt->second);
	else
		envVars.push_back("CONTENT_TYPE=");

	// Set CONTENT_LENGTH only for POST requests, empty for GET
	if (req.method == "POST")
	{
		std::ostringstream contentLength;
		contentLength << req.body.size();
		envVars.push_back("CONTENT_LENGTH=" + contentLength.str());
	}
	else
		envVars.push_back("CONTENT_LENGTH=");

	// Extract SERVER_NAME and SERVER_PORT from Host header, or use defaults
	std::string serverName = "localhost";
	std::string serverPort = "8080";
	std::map<std::string, std::string>::const_iterator hostIt = req.headers.find("host");
	if (hostIt != req.headers.end())
	{
		std::string host = hostIt->second;
		size_t colonPos = host.find(':');
		if (colonPos != std::string::npos)
		{
			serverName = host.substr(0, colonPos);
			serverPort = host.substr(colonPos + 1);
		}
		else
			serverName = host;
	}
	
	envVars.push_back("SERVER_NAME=" + serverName);
	envVars.push_back("SERVER_PORT=" + serverPort);
	envVars.push_back("SERVER_PROTOCOL=" + req.version);
	envVars.push_back("SERVER_SOFTWARE=webserv/1.0");
	envVars.push_back("REQUEST_URI=" + req.path);
	envVars.push_back("GATEWAY_INTERFACE=CGI/1.1");
	envVars.push_back("REMOTE_HOST=localhost");
	envVars.push_back("REMOTE_ADDR=" + (req.clientAddr.empty() ? std::string("127.0.0.1") : req.clientAddr));
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

    std::time_t startTime = std::time(NULL);

    while (true)
    {
        // Check if total time has exceeded timeout
        std::time_t elapsed = std::time(NULL) - startTime;
        if (elapsed >= timeoutSeconds)
        {
            kill(pid, SIGKILL);
            throw std::runtime_error("CGI Timeout");
        }
        
        // Calculate remaining time for this poll call
        int remainingSeconds = timeoutSeconds - elapsed;
        int pollTimeoutMs = (remainingSeconds > 0) ? (remainingSeconds * 1000) : 0;
        if (pollTimeoutMs > 1000)
            pollTimeoutMs = 1000; // Cap to 1 second per poll to allow checking elapsed time

        int ret = poll(&pfd, 1, pollTimeoutMs);
        if (ret == 0)
        {
            // Timeout in poll - continue loop to check total elapsed time
            continue;
        }
        else if (ret < 0)
        {
            throw std::runtime_error("Poll error");
        }

        ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
        if (bytesRead > 0)
            output.append(buffer, bytesRead);
        else
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

static bool setNonBlockingFd(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
		return false;
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		return false;
	return true;
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

Response CGIHandler::buildCgiResponse(const std::string &cgiOutput, const std::string &scriptPath)
{
	size_t headerEnd = cgiOutput.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		headerEnd = cgiOutput.find("\n\n");
	return buildCgiRes(headerEnd, cgiOutput, scriptPath);
}

bool CGIHandler::startCgiProcess(const Request &req,
								const LocationConfig &config,
								const std::string &scriptPath,
								CgiProcess &proc,
								Response &errorRes)
{
	int inPipe[2];
	int outPipe[2];

	struct stat st;
	if (stat(scriptPath.c_str(), &st) != 0)
	{
		errorRes = ErrorHandler::handleError(404, "Not Found", config);
		return false;
	}
	if (!S_ISREG(st.st_mode))
	{
		errorRes = ErrorHandler::handleError(403, "Forbidden", config);
		return false;
	}

	// Get interpreter (if any) - check this early
	std::string exten = getFileExt(scriptPath, config);
	bool useInterpreter = !exten.empty();
	
	// If using interpreter, verify interpreter exists before checking script perms
	if (useInterpreter && stat(exten.c_str(), &st) != 0)
	{
		errorRes = ErrorHandler::handleError(500, "Internal Server Error", config);
		return false;
	}

	// Check script permissions based on execution method
	if (!isExecutable(scriptPath, useInterpreter))
	{
		errorRes = ErrorHandler::handleError(403, "Forbidden", config);
		return false;
	}

	std::vector<std::string> envVars = buildEnvVars(req, scriptPath, config);
	char **envArray = envToArray(envVars);

	if (pipe(inPipe) == -1 || pipe(outPipe) == -1)
	{
		freeEnvArray(envArray);
		errorRes = ErrorHandler::handleError(500, "Internal Server Error", config);
		return false;
	}

	pid_t pid = fork();
	if (pid == -1)
	{
		closePipes(inPipe, outPipe);
		freeEnvArray(envArray);
		errorRes = ErrorHandler::handleError(500, "Internal Server Error", config);
		return false;
	}

	if (pid == 0)
	{
		// Child process
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
		if (useInterpreter)
		{
			argv[0] = const_cast<char*>(exten.c_str());
			argv[1] = const_cast<char*>(absScriptPath.c_str());
			argv[2] = NULL;
			execve(exten.c_str(), argv, envArray);
		}
		else
		{
			argv[0] = const_cast<char*>(absScriptPath.c_str());
			argv[1] = NULL;
			execve(absScriptPath.c_str(), argv, envArray);
		}
		_exit(127);
	}

	// Parent process
	close(inPipe[0]);
	close(outPipe[1]);

	if (!setNonBlockingFd(inPipe[1]) || !setNonBlockingFd(outPipe[0]))
	{
		close(inPipe[1]);
		close(outPipe[0]);
		kill(pid, SIGKILL);
		int status;
		waitpid(pid, &status, 0);
		freeEnvArray(envArray);
		errorRes = ErrorHandler::handleError(500, "Internal Server Error", config);
		return false;
	}

	proc.inFd = inPipe[1];
	proc.outFd = outPipe[0];
	proc.pid = pid;
	proc.input = req.body;
	proc.inputOffset = 0;
	proc.output.clear();
	proc.startTime = std::time(NULL);
	proc.timeoutSec = 30;
	proc.scriptPath = scriptPath;

	// Free envArray in parent - child will have already exec'd or exited
	freeEnvArray(envArray);
	return true;
}
