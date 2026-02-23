/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/25 22:49:08 by marvin            #+#    #+#             */
/*   Updated: 2026/01/25 22:49:08 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server/Logger.hpp"
#include <sstream>



void Logger::info(const std::string& msg)
{
    std::cout<< "[INFO]: " << msg << std::endl;
}
void Logger::error(const std::string& msg)
{
    std::cout << "[ERROR]: " << msg << std::endl;
}

std::string Logger::toStringInt(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}
