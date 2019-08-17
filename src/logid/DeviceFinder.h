#ifndef DEVICEFINDER_H
#define DEVICEFINDER_H

#include <hid/DeviceMonitor.h>
#include <hidpp/SimpleDispatcher.h>
#include <hidpp10/Device.h>
#include <hidpp10/IReceiver.h>
#include <hidpp20/IReprogControls.h>
#include <map>
#include <thread>
#include "Device.h"

class Device;

class DeviceFinder : public HID::DeviceMonitor
{
public:
    std::map<Device*, std::thread> devices;
protected:
    void addDevice(const char* path);
    void removeDevice(const char* path);
};

extern DeviceFinder* finder;

#endif //DEVICEFINDER_H