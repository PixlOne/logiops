#ifndef MASTEROPTIONS_DEVICEFINDER_H
#define MASTEROPTIONS_DEVICEFINDER_H

#include "Device.h"

struct handler_pair;

class DeviceFinder : public HID::DeviceMonitor
{
protected:
    void addDevice(const char* path);
    void removeDevice(const char* path);
    std::map<Device*, std::future<void>> handlers;
};

void find_device();

#endif //MASTEROPTIONS_DEVICEFINDER_H
