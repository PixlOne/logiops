#include "util.h"
#include <iostream>

using namespace logid::IPC;

deviceInfo logid::IPC::parseDevName(std::string name)
{
    auto it = name.find(':');
    if(it == std::string::npos)
        throw std::invalid_argument("Invalid device name!");
    else if(it == name.size()-1)
        throw std::invalid_argument("No device number specified!");

    deviceInfo info;
    info.path = name.substr(0, it);
    std::string index_str = name.substr(it+1);
    try
    {
        int i = std::stoi(index_str);
        if(i > 6 && i != 255)
            throw std::invalid_argument("");
        info.index = static_cast<HIDPP::DeviceIndex>(i);
    } catch(std::invalid_argument &e) {
        throw std::invalid_argument("Invalid device number!");
    }

    return info;
}

std::string logid::IPC::toDevName(deviceInfo info)
{
    return info.path + ':' + std::to_string(info.index);
}

std::string logid::IPC::toDevName(std::string path, HIDPP::DeviceIndex index)
{
    return path + ':' + std::to_string(index);
}