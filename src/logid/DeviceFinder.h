#ifndef DEVICEFINDER_H
#define DEVICEFINDER_H

#include <hid/DeviceMonitor.h>
#include <hidpp/SimpleDispatcher.h>
#include <hidpp10/Device.h>
#include <hidpp10/IReceiver.h>
#include <hidpp20/IReprogControls.h>
#include <map>
#include <thread>
#include <mutex>

#include "Device.h"

#define MAX_CONNECTION_TRIES 10
#define TIME_BETWEEN_CONNECTION_TRIES 500ms

class Device;

struct ConnectedDevice {
	Device *device;
	std::thread associatedThread;
};

class DeviceFinder : public HID::DeviceMonitor
{
public:
	~DeviceFinder();

	Device* insertNewDevice (const std::string &path, HIDPP::DeviceIndex index);
	Device* insertNewReceiverDevice (const std::string &path, HIDPP::DeviceIndex index);
	void stopAndDeleteAllDevicesIn (const std::string &path);
	void stopAndDeleteDevice (const std::string &path, HIDPP::DeviceIndex index);
protected:
    void addDevice(const char* path);
    void removeDevice(const char* path);
private:
	std::mutex devices_mutex;
    std::map<std::string, std::map<HIDPP::DeviceIndex, ConnectedDevice>> devices;
};

extern DeviceFinder* finder;

#endif //DEVICEFINDER_H