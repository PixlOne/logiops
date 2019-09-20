#pragma once

#include <hid/DeviceMonitor.h>
#include <hidpp/SimpleDispatcher.h>
#include <hidpp10/Device.h>
#include <hidpp10/IReceiver.h>
#include <hidpp20/IReprogControls.h>
#include <unordered_map>
#include <list>
#include <thread>
#include <mutex>

#include "Device.h"

class Device;

struct PairedDevice {
	Device *device;
	std::thread associatedThread;
};

class DeviceFinder : public HID::DeviceMonitor
{
public:
	~DeviceFinder();

	void insertNewDevice (const std::string &path, HIDPP::DeviceIndex index);
	void stopAndDeleteAllDevicesIn (const std::string &path);
	void stopAndDeleteDevice (const std::string &path, HIDPP::DeviceIndex index);
protected:
    void addDevice(const char* path);
    void removeDevice(const char* path);
private:
	std::mutex devicesMutex;
    std::map<std::string, std::map<HIDPP::DeviceIndex, PairedDevice>> devices;
};

extern DeviceFinder* finder;

