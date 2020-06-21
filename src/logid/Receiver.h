#ifndef LOGID_RECEIVER_H
#define LOGID_RECEIVER_H

#include <string>
#include "backend/dj/ReceiverMonitor.h"
#include "Device.h"

namespace logid
{
    class Receiver : public backend::dj::ReceiverMonitor
    {
    public:
        Receiver(std::string path);

    protected:
        virtual void addDevice(backend::hidpp::DeviceConnectionEvent event);
        virtual void removeDevice(backend::hidpp::DeviceIndex index);
    private:
        std::map<backend::hidpp::DeviceIndex, std::shared_ptr<Device>> _devices;
        std::string _path;
    };
}

#endif //LOGID_RECEIVER_H