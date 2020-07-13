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

std::string logid::IPC::toDBusPath(logid::Device* device)
{
    return toDBusPath(device->path, device->index);
}

std::string logid::IPC::toDBusPath(std::string path, HIDPP::DeviceIndex index)
{
    std::string dev_path = path;
    if(dev_path.back() == '/')
        dev_path.erase(dev_path.end()-1);
    auto it = dev_path.rfind('/');
    std::string dev_trunc_name;
    if(it == std::string::npos)
        dev_trunc_name = dev_path;
    else
        dev_trunc_name = dev_path.substr(it+1);
    for(char & i : dev_trunc_name)
        if((i < 'A' || i > 'Z') &&
           (i < 'a' || i > 'z') &&
           (i < '0' || i > '9') &&
           i != '_')
            i = '_';

    return "/pizza/pixl/logiops/device/" + dev_trunc_name + "/" + std::to_string(index);
}