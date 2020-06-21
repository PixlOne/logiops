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
namespace dj
{
    // Need to define here for a constructor
    class Receiver;
}
namespace hidpp
{
    struct DeviceConnectionEvent;
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
                InvalidRawDevice,
                Asleep
            };
            InvalidDevice(Reason reason) : _reason (reason) {}
            virtual const char *what() const noexcept;
            virtual Reason code() const noexcept;
        private:
            Reason _reason;
        };

        explicit Device(const std::string& path, DeviceIndex index);
        explicit Device(std::shared_ptr<raw::RawDevice> raw_device, DeviceIndex index);
        explicit Device(std::shared_ptr<dj::Receiver> receiver,
                hidpp::DeviceConnectionEvent event);
        ~Device();

        std::string devicePath() const { return _path; }
        DeviceIndex deviceIndex() const { return _index; }
        std::tuple<uint8_t, uint8_t> version() const { return _version; }

        void listen(); // Runs asynchronously
        void stopListening();

        void addEventHandler(const std::string& nickname,
                const std::shared_ptr<EventHandler>& handler);
        void removeEventHandler(const std::string& nickname);

        Report sendReport(Report& report);

        void handleEvent(Report& report);
    private:
        void _init();

        std::shared_ptr<raw::RawDevice> _raw_device;
        std::shared_ptr<dj::Receiver> _receiver;
        std::string _path;
        DeviceIndex _index;
        uint8_t supported_reports;

        std::tuple<uint8_t, uint8_t> _version;
        uint16_t _pid;
        std::string _name;

        std::map<std::string, std::shared_ptr<EventHandler>> event_handlers;
    };
} } }

#endif //LOGID_HIDPP_DEVICE_H