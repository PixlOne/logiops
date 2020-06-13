#include "Device.h"
#include "util.h"

using namespace logid;

IPC::Device::Device(DBus::Connection &connection, logid::Device* device): DBus::ObjectAdaptor(
        connection, IPC::toDBusPath(device)), _device(device)
{
    Name = device->name;
    std::vector<std::string> features;
    /// TODO: Set features
    Features = features;
    DeviceID = device->pid;
}

void IPC::Device::GetInfo(const std::string& device)
{

}
