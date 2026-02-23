#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>


class Logger
{
    public:
        static void info(const std::string& msg);
        static void error(const std::string& msg);
        static std::string toStringInt(int n);
};

#endif
