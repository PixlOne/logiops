#ifndef LOGID_DEVICEMONITOR_H
#define LOGID_DEVICEMONITOR_H

#include <map>
#include <thread>
#include <mutex>

#include "backend/raw/DeviceMonitor.h"
#include "backend/hidpp/Device.h"
#include "Device.h"
#include "Receiver.h"

namespace logid
{

    class DeviceMonitor : public backend::raw::DeviceMonitor
    {
    public:
        DeviceMonitor() = default;
    protected:
        void addDevice(std::string path) override;
        void removeDevice(std::string path) override;
    private:

        std::map<std::string, std::shared_ptr<Device>> _devices;
        std::map<std::string, std::shared_ptr<Receiver>> _receivers;
    };

    extern DeviceMonitor* finder;
}

#endif //LOGID_DEVICEFINDER_H