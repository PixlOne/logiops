#include "util.h"
#include "Device.h"

using namespace logid;

Device::Device(std::string path, backend::hidpp::DeviceIndex index) :
    _hidpp20 (path, index), _path (path), _index (index)
{
    log_printf(DEBUG, "logid::Device created on %s:%d", _path.c_str(), _index);
}

void Device::sleep()
{
    log_printf(INFO, "%s:%d fell asleep.", _path.c_str(), _index);
}

void Device::wakeup()
{
    log_printf(INFO, "%s:%d woke up.", _path.c_str(), _index);
}
