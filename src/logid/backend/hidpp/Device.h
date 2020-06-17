#ifndef LOGID_HIDPP_DEVICE_H
#define LOGID_HIDPP_DEVICE_H

#include <string>
#include <memory>
#include <functional>
#include <map>
#include "../raw/RawDevice.h"
#include "Report.h"
#include "defs.h"

namespace logid {
namespace backend {
namespace hidpp
{
    struct EventHandler
    {
        std::function<bool(Report&)> condition;
        std::function<void(Report&)> callback;
    };
    class Device
    {
    public:
        class InvalidDevice : std::exception
        {
        public:
            enum Reason
            {
                NoHIDPPReport,
                InvalidRawDevice
            };
            InvalidDevice(Reason reason) : _reason (reason) {}
            virtual const char *what() const noexcept;
            virtual Reason code() const noexcept;
        private:
            Reason _reason;
        };

        Device(const std::string& path, DeviceIndex index);

        std::string devicePath() const { return path; }
        DeviceIndex deviceIndex() const { return index; }

        void listen(); // Runs asynchronously
        void stopListening();

        void addEventHandler(const std::string& nickname, EventHandler& handler);
        void removeEventHandler(const std::string& nickname);

        Report sendReport(Report& report);

        void handleEvent(Report& report);
    private:
        std::shared_ptr<raw::RawDevice> raw_device;
        std::string path;
        DeviceIndex index;
        uint8_t supported_reports;

        std::map<std::string, EventHandler> event_handlers;
    };
} } }

#endif //LOGID_HIDPP_DEVICE_H