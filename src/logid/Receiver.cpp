#include <cassert>
#include "Receiver.h"
#include "util.h"
#include "backend/hidpp10/Error.h"
#include "backend/hidpp20/Error.h"

using namespace logid;
using namespace logid::backend;

Receiver::Receiver(std::string path) : dj::ReceiverMonitor(path)
{
    log_printf(DEBUG, "logid::Receiver created on %s", path.c_str());
}

void Receiver::addDevice(hidpp::DeviceConnectionEvent event)
{
    try {
        if(!event.linkEstablished)
            return; // Device is probably asleep, wait until it wakes up

        hidpp::Device hidpp_device(receiver(), event);

        auto version = hidpp_device.version();

        if(std::get<0>(version) < 2) {
            log_printf(INFO, "Unsupported HID++ 1.0 device on %s:%d connected.",
                    _path.c_str(), event.index);
            return;
        }

        std::shared_ptr<Device> device = std::make_shared<Device>(
                receiver()->rawDevice(), event.index);

        assert(_devices.find(event.index) == _devices.end());

        _devices.emplace(event.index, device);

    } catch(hidpp10::Error &e) {
        log_printf(ERROR, "Caught HID++ 1.0 error while trying to initialize "
                          "%s:%d: %s", _path.c_str(), event.index, e.what());
    } catch(hidpp20::Error &e) {
        log_printf(ERROR, "Caught HID++ 2.0 error while trying to initialize "
                          "%s:%d: %s", _path.c_str(), event.index, e.what());
    }
}

void Receiver::removeDevice(hidpp::DeviceIndex index)
{
    _devices.erase(index);
}