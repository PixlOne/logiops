#ifndef LOGID_BACKEND_RAW_DEVICEMONITOR_H
#define LOGID_BACKEND_RAW_DEVICEMONITOR_H

#include <string>
#include <mutex>
#include <atomic>

extern "C"
{
#include <libudev.h>
}

namespace logid {
namespace backend {
namespace raw
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
        struct udev* _udev_context;
        int _pipe[2];
        std::atomic<bool> _run_monitor;
        std::mutex _running;
    };
}}}

#endif //LOGID_BACKEND_RAW_DEVICEMONITOR_H