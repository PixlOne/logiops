#ifndef LOGID_BACKEND_RAW_DEVICEMONITOR_H
#define LOGID_BACKEND_RAW_DEVICEMONITOR_H

#include <string>
#include <mutex>

extern "C"
{
#include <libudev.h>
}

namespace logid::backend::raw
{
    class DeviceMonitor
    {
    public:
        void enumerate();
        void run();
        void stop();
    protected:
        DeviceMonitor();
        ~DeviceMonitor();
        virtual void addDevice(std::string device) = 0;
        virtual void removeDevice(std::string device) = 0;
    private:
        struct udev* udev_context;
        int monitor_pipe[2];
        std::mutex running;
    };
}

#endif //LOGID_BACKEND_RAW_DEVICEMONITOR_H