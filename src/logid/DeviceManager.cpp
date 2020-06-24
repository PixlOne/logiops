#include <thread>
#include <sstream>

#include "DeviceManager.h"
#include "Receiver.h"
#include "util.h"
#include "backend/hidpp10/Error.h"
#include "backend/dj/Receiver.h"

#define NON_WIRELESS_DEV(index) (index) == HIDPP::DefaultDevice ? "default" : "corded"

using namespace logid;
using namespace logid::backend;

void DeviceManager::addDevice(std::string path)
{
    bool defaultExists = true;
    bool isReceiver = false;
    try {
        hidpp::Device device(path, hidpp::DefaultDevice);
        isReceiver = device.version() == std::make_tuple(1, 0);
    } catch(hidpp10::Error &e) {
        if(e.code() != hidpp10::Error::UnknownDevice)
            throw;
    } catch(hidpp::Device::InvalidDevice &e) { // Ignore
        defaultExists = false;
    } catch(std::system_error &e) {
        log_printf(WARN, "I/O error on %s: %s, skipping device.",
                path.c_str(), e.what());
        return;
    }

    if(isReceiver) {
        log_printf(INFO, "Detected receiver at %s", path.c_str());
        auto receiver = std::make_shared<Receiver>(path);
        receiver->run();
        _receivers.emplace(path, receiver);
    } else {
        /* TODO: Error check?
         * TODO: Can non-receivers only contain 1 device?
         * If the device exists, it is guaranteed to be an HID++ 2.0 device */
        if(defaultExists) {
            auto device = std::make_shared<Device>(path, hidpp::DefaultDevice);
            _devices.emplace(path,  device);
        } else {
            try {
                auto device = std::make_shared<Device>(path,
                        hidpp::CordedDevice);
                _devices.emplace(path, device);
            } catch(hidpp10::Error &e) {
                if(e.code() != hidpp10::Error::UnknownDevice)
                    throw;
                else
                    log_printf(WARN,
                            "HID++ 1.0 error while trying to initialize %s:"
                            "%s", path.c_str(), e.what());
            } catch(hidpp::Device::InvalidDevice &e) { // Ignore
            } catch(std::system_error &e) {
                // This error should have been thrown previously
                log_printf(WARN, "I/O error on %s: %s", path.c_str(),
                        e.what());
            }
        }
    }
}

void DeviceManager::removeDevice(std::string path)
{
    auto receiver = _receivers.find(path);

    if(receiver != _receivers.end()) {
        _receivers.erase(receiver);
        log_printf(INFO, "Receiver on %s disconnected", path.c_str());
    } else {
        auto device = _devices.find(path);
        if(device != _devices.find(path)) {
            _devices.erase(device);
            log_printf(INFO, "Device on %s disconnected", path.c_str());
        }
    }
}
