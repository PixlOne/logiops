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
    static constexpr uint16_t UnifyingReceivers[] =
            {
                0xc52b, 0xc532, // Official Unifying receivers
                0xc52f, 0xc526, // Nano receivers
                0xc52e, 0xc51b,
                0xc531, 0xc517,
                0xc518, 0xc51a,
                0xc521, 0xc525,
                0xc534,
                0xc539, 0xc53a, // Lightspeed receivers
                0xc53f,
                0x17ef,         // Lenovo nano receivers
            };
protected:
    void addDevice(const char* path);
    void removeDevice(const char* path);
};

extern DeviceFinder* finder;

#endif //DEVICEFINDER_H