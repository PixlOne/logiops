#ifndef LOGID_DEVICEMANAGER_H
#define LOGID_DEVICEMANAGER_H

#include <map>
#include <thread>
#include <mutex>

#include "backend/raw/DeviceMonitor.h"
#include "backend/hidpp/Device.h"
#include "Device.h"
#include "Receiver.h"

namespace logid
{

    class DeviceManager : public backend::raw::DeviceMonitor
    {
    public:
        DeviceManager() = default;
    protected:
        void addDevice(std::string path) override;
        void removeDevice(std::string path) override;
    private:

        std::map<std::string, std::shared_ptr<Device>> _devices;
        std::map<std::string, std::shared_ptr<Receiver>> _receivers;
    };

    extern DeviceManager* finder;
}

#endif //LOGID_DEVICEMANAGER_H