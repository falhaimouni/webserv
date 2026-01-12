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

static int g_cgiPid = -1;

static void cgiTimeoutHandler(int sig)
{
    (void)sig;
    if (g_cgiPid > 0)
    {
        kill(g_cgiPid, SIGKILL);
        g_cgiPid = -1;
    }
}

std::string CGIHandler::getInterpreter(const std::string &scriptPath, const LocationConfig &config)
{
    // Find file extension
    size_t dotPos = scriptPath.rfind('.');
    if (dotPos == std::string::npos)
        return "";
    
    std::string ext = scriptPath.substr(dotPos);
    
    // Check if extension is in CGI extensions
    if (config.cgi_ext.find(ext) == config.cgi_ext.end())
        return "";
    
    // Map extensions to interpreters
    if (ext == ".py" || ext == ".py3")
        return "/usr/bin/python3";
    else if (ext == ".php")
        return "/usr/bin/php";
    else if (ext == ".pl")
        return "/usr/bin/perl";
    else if (ext == ".sh")
        return "/usr/bin/bash";  // Fixed: use /usr/bin/bash instead of /bin/bash
    else if (ext == ".rb")
        return "/usr/bin/ruby";
    
    return "";
}

bool CGIHandler::isExecutable(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    return (st.st_mode & S_IXUSR) != 0;
}

std::vector<std::string> CGIHandler::buildEnvVars(const Request &req, const std::string &scriptPath, const LocationConfig &config)
{
    (void)config; // May be used for future configuration options
    std::vector<std::string> envVars;
    
    // Standard CGI variables
    envVars.push_back("REQUEST_METHOD=" + req.method);
    envVars.push_back("SCRIPT_NAME=" + req.path);
    envVars.push_back("PATH_INFO=" + req.path);
    envVars.push_back("QUERY_STRING=");
    
    // Extract query string from path if present
    size_t queryPos = req.path.find('?');
    if (queryPos != std::string::npos)
    {
        envVars[envVars.size() - 1] = "QUERY_STRING=" + req.path.substr(queryPos + 1);
        envVars[envVars.size() - 2] = "SCRIPT_NAME=" + req.path.substr(0, queryPos);
        envVars[envVars.size() - 2] = "PATH_INFO=" + req.path.substr(0, queryPos);
    }
    
    // Content type and length (headers are stored lowercase)
    std::map<std::string, std::string>::const_iterator contentTypeIt = req.headers.find("content-type");
    if (contentTypeIt != req.headers.end())
        envVars.push_back("CONTENT_TYPE=" + contentTypeIt->second);
    else
        envVars.push_back("CONTENT_TYPE=");
    
    std::ostringstream contentLength;
    contentLength << req.body.size();
    envVars.push_back("CONTENT_LENGTH=" + contentLength.str());
    
    // Server variables
    envVars.push_back("SERVER_NAME=localhost");
    envVars.push_back("SERVER_PORT=8080");
    envVars.push_back("SERVER_PROTOCOL=HTTP/1.0");
    envVars.push_back("SERVER_SOFTWARE=webserv/1.0");
    
    // Request URI
    envVars.push_back("REQUEST_URI=" + req.path);
    
    // Gateway interface
    envVars.push_back("GATEWAY_INTERFACE=CGI/1.1");
    
    // Remote address (simplified)
    envVars.push_back("REMOTE_ADDR=127.0.0.1");
    envVars.push_back("REMOTE_HOST=localhost");
    
    // Script filename (full path)
    char resolvedPath[4096];
    if (realpath(scriptPath.c_str(), resolvedPath) != NULL)
        envVars.push_back("SCRIPT_FILENAME=" + std::string(resolvedPath));
    else
        envVars.push_back("SCRIPT_FILENAME=" + scriptPath);
    
    // HTTP headers as environment variables
    for (std::map<std::string, std::string>::const_iterator it = req.headers.begin();
         it != req.headers.end(); ++it)
    {
        std::string headerName = it->first;
        // Convert to HTTP_ format (uppercase, replace - with _)
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

std::string CGIHandler::readFromPipe(int fd, int timeoutSeconds)
{
    std::string output;
    const size_t BUFFER_SIZE = 65536;  // Increased from 4096 for better throughput
    char *buffer = new char[BUFFER_SIZE];
    ssize_t bytesRead;
    
    // Set up timeout using alarm
    signal(SIGALRM, cgiTimeoutHandler);
    alarm(timeoutSeconds);
    
    while (true)
    {
        bytesRead = read(fd, buffer, BUFFER_SIZE);
        if (bytesRead > 0)
        {
            output.append(buffer, bytesRead);
        }
        else if (bytesRead == 0)
        {
            // EOF reached
            break;
        }
        else
        {
            // Error occurred
            if (errno == EINTR)  // Interrupted by signal, try again
                continue;
            break;
        }
    }
    
    alarm(0); // Cancel alarm
    signal(SIGALRM, SIG_DFL);
    delete[] buffer;
    
    return output;
}

Response CGIHandler::handleCGIReq(const std::string &scriptPath, const std::string &input)
{
    Response res;
    (void)input;
    // Simple simulation for backward compatibility
    std::ifstream scriptFile(scriptPath.c_str());
    if (!scriptFile.is_open())
    {
        return ErrorHandler::handleError(500, "Internal Server Error");
    }
    
    std::string line;
    std::string output;
    while (std::getline(scriptFile, line))
    {
        output += line + "\n";
    }
    scriptFile.close();
    
    res.status = 200;
    res.reason = "OK";
    res.headers["Content-Type"] = "text/html";
    res.body = output;
    
    std::stringstream len;
    len << res.body.size();
    res.headers["Content-Length"] = len.str();
    return res;
}

Response CGIHandler::handleCGIReq(const Request &req, const LocationConfig &config, const std::string &scriptPath)
{
    std::cerr << "CGI: scriptPath = " << scriptPath << std::endl;
    
    // Check if script exists and is readable
    struct stat st;
    if (stat(scriptPath.c_str(), &st) != 0)
    {
        std::cerr << "CGI: Script not found: " << scriptPath << std::endl;
        return ErrorHandler::handleError(404, "Not Found");
    }
    
    if (!S_ISREG(st.st_mode))
    {
        std::cerr << "CGI: Not a regular file: " << scriptPath << std::endl;
        return ErrorHandler::handleError(403, "Forbidden");
    }
    
    // Get interpreter
    std::string interpreter = getInterpreter(scriptPath, config);
    std::cerr << "CGI: interpreter = " << (interpreter.empty() ? "(direct)" : interpreter) << std::endl;
    std::cerr << "CGI: isExecutable = " << (isExecutable(scriptPath) ? "yes" : "no") << std::endl;
    
    // If script is executable and has shebang, execute directly instead of using interpreter
    if (isExecutable(scriptPath))
    {
        std::cerr << "CGI: Script is executable, will execute directly" << std::endl;
        interpreter = "";  // Execute directly, let the kernel use the shebang
    }
    else if (interpreter.empty())
    {
        std::cerr << "CGI: Script not executable and no interpreter found" << std::endl;
        return ErrorHandler::handleError(500, "Internal Server Error");
    }
    else
    {
        // Check if interpreter exists
        if (stat(interpreter.c_str(), &st) != 0)
        {
            std::cerr << "CGI: Interpreter not found: " << interpreter << std::endl;
            return ErrorHandler::handleError(500, "Internal Server Error");
        }
    }
    
    // Build environment variables
    std::vector<std::string> envVars = buildEnvVars(req, scriptPath, config);
    char **envArray = envToArray(envVars);
    
    // Create pipes for stdin, stdout, stderr
    int stdinPipe[2];
    int stdoutPipe[2];
    
    if (pipe(stdinPipe) == -1 || pipe(stdoutPipe) == -1)
    {
        freeEnvArray(envArray);
        return ErrorHandler::handleError(500, "Internal Server Error");
    }
    
    // Fork process
    pid_t pid = fork();
    if (pid == -1)
    {
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        freeEnvArray(envArray);
        return ErrorHandler::handleError(500, "Internal Server Error");
    }
    
    if (pid == 0)
    {
        // Child process
        g_cgiPid = getpid();
        
        // Redirect stdin, stdout, stderr
        dup2(stdinPipe[0], STDIN_FILENO);
        dup2(stdoutPipe[1], STDOUT_FILENO);
        dup2(stdoutPipe[1], STDERR_FILENO);
        
        // Close unused pipe ends
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        
        // Get absolute path before changing directory
        char resolvedScriptPath[PATH_MAX];
        if (realpath(scriptPath.c_str(), resolvedScriptPath) == NULL)
        {
            std::cerr << "CGI: Child - failed to resolve script path: " << scriptPath << std::endl;
            _exit(127);
        }
        std::string absScriptPath(resolvedScriptPath);
        
        // Change to script directory (for relative paths in the script)
        size_t lastSlash = absScriptPath.find_last_of('/');
        if (lastSlash != std::string::npos)
        {
            std::string scriptDir = absScriptPath.substr(0, lastSlash);
            chdir(scriptDir.c_str());
        }
        
        // Prepare arguments using absolute path
        char *argv[3];
        if (interpreter.empty())
        {
            // Direct execution
            std::cerr << "CGI: Child - executing directly: " << absScriptPath << std::endl;
            argv[0] = const_cast<char*>(absScriptPath.c_str());
            argv[1] = NULL;
            execve(absScriptPath.c_str(), argv, envArray);
        }
        else
        {
            // Execute with interpreter
            std::cerr << "CGI: Child - executing with interpreter: " << interpreter << " " << absScriptPath << std::endl;
            argv[0] = const_cast<char*>(interpreter.c_str());
            argv[1] = const_cast<char*>(absScriptPath.c_str());
            argv[2] = NULL;
            execve(interpreter.c_str(), argv, envArray);
        }
        
        // If execve fails
        std::cerr << "CGI: Child - execve failed: " << strerror(errno) << std::endl;
        _exit(127);
    }
    else
    {
        // Parent process
        g_cgiPid = pid;
        
        std::cerr << "CGI: Parent - closing unused pipe ends" << std::endl;
        
        // Close unused pipe ends
        close(stdinPipe[0]);
        close(stdoutPipe[1]);
        
        std::cerr << "CGI: Parent - writing to stdin" << std::endl;
        
        // Write request body to CGI stdin
        if (!req.body.empty())
        {
            write(stdinPipe[1], req.body.c_str(), req.body.size());
        }
        close(stdinPipe[1]);
        
        std::cerr << "CGI: Parent - reading from stdout" << std::endl;
        
        // Read CGI output with timeout (30 seconds)
        std::string cgiOutput = readFromPipe(stdoutPipe[0], 30);
        close(stdoutPipe[0]);
        
        std::cerr << "CGI: Parent - waiting for child" << std::endl;;
        
        // Wait for child process
        int status;
        waitpid(pid, &status, 0);
        g_cgiPid = -1;
        
        freeEnvArray(envArray);
        
        // Check if process exited normally
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            std::cerr << "CGI: Process exited with status " << WEXITSTATUS(status) << std::endl;
            return ErrorHandler::handleError(500, "Internal Server Error");
        }
        
        if (WIFSIGNALED(status))
        {
            std::cerr << "CGI: Process killed by signal " << WTERMSIG(status) << std::endl;
            return ErrorHandler::handleError(504, "Gateway Timeout");
        }
        
        std::cerr << "CGI: Output length = " << cgiOutput.size() << " bytes" << std::endl;
        
        // Parse CGI output
        // CGI output format: headers (optional) + blank line + body
        size_t headerEnd = cgiOutput.find("\r\n\r\n");
        if (headerEnd == std::string::npos)
            headerEnd = cgiOutput.find("\n\n");
        
        Response res;
        res.status = 200;
        res.reason = "OK";
        
        if (headerEnd != std::string::npos)
        {
            // Parse headers
            std::string headers = cgiOutput.substr(0, headerEnd);
            size_t bodyStart = headerEnd + (cgiOutput.find("\r\n\r\n") != std::string::npos ? 4 : 2);
            res.body = cgiOutput.substr(bodyStart);
            
            // Parse header lines
            std::istringstream headerStream(headers);
            std::string headerLine;
            while (std::getline(headerStream, headerLine))
            {
                size_t colonPos = headerLine.find(':');
                if (colonPos != std::string::npos)
                {
                    std::string key = headerLine.substr(0, colonPos);
                    std::string value = headerLine.substr(colonPos + 1);
                    
                    // Trim whitespace
                    while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
                        value = value.substr(1);
                    while (!value.empty() && (value[value.size() - 1] == '\r' || value[value.size() - 1] == '\n'))
                        value = value.substr(0, value.size() - 1);
                    
                    if (key == "Status")
                    {
                        // Parse status line (e.g., "200 OK")
                        size_t spacePos = value.find(' ');
                        if (spacePos != std::string::npos)
                        {
                            std::istringstream statusStream(value.substr(0, spacePos));
                            statusStream >> res.status;
                            res.reason = value.substr(spacePos + 1);
                        }
                    }
                    else
                    {
                        res.headers[key] = value;
                    }
                }
            }
        }
        else
        {
            // No headers, entire output is body
            res.body = cgiOutput;
            res.headers["Content-Type"] = "text/html";
        }
        
        // Ensure Content-Type is set
        if (res.headers.find("Content-Type") == res.headers.end())
        {
            res.headers["Content-Type"] = getType::getMimeType(scriptPath);
        }
        
        std::stringstream len;
        len << res.body.size();
        res.headers["Content-Length"] = len.str();
        
        return res;
    }
}