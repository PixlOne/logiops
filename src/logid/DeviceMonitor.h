#ifndef LOGID_DEVICEMONITOR_H
#define LOGID_DEVICEMONITOR_H

#include <map>
#include <thread>
#include <mutex>

#include "backend/raw/DeviceMonitor.h"
#include "backend/hidpp/Device.h"

#define MAX_CONNECTION_TRIES 10
#define TIME_BETWEEN_CONNECTION_TRIES 500ms

namespace logid
{

    class Device;

    struct ConnectedDevice {
    	Device *device;
    	std::thread associatedThread;
    };

    class DeviceMonitor : public backend::raw::DeviceMonitor
    {
    public:
    	~DeviceMonitor();

    	/*
    	Device* insertNewDevice (const std::string &path, HIDPP::DeviceIndex index);
    	Device* insertNewReceiverDevice (const std::string &path, HIDPP::DeviceIndex index);
    	void stopAndDeleteAllDevicesIn (const std::string &path);
    	void stopAndDeleteDevice (const std::string &path, HIDPP::DeviceIndex index);
    	 */
    protected:
        void addDevice(std::string path) override;
        void removeDevice(std::string path) override;
    private:
    	std::mutex devices_mutex;
    	std::vector<std::shared_ptr<backend::hidpp::Device>> devices; //tmp
        //std::map<std::string, std::map<backend::hidpp::DeviceIndex, ConnectedDevice>> devices;
    };

    extern DeviceMonitor* finder;
}

#endif //LOGID_DEVICEFINDER_H