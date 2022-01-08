/*
 * Copyright 2019-2020 PixlOne
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <thread>
#include <sstream>
#include <utility>

#include "DeviceManager.h"
#include "Receiver.h"
#include "util/log.h"
#include "util/workqueue.h"
#include "backend/hidpp10/Error.h"
#include "backend/Error.h"

using namespace logid;
using namespace logid::backend;

namespace logid {
    class _DeviceManager : public logid::DeviceManager
    {
    public:
        template <typename... Args>
        explicit _DeviceManager(Args... args) :
            DeviceManager(std::forward<Args>(args)...) { }
    };
}

DeviceManager::DeviceManager(std::shared_ptr<Configuration> config,
                             std::shared_ptr<InputDevice> virtual_input) :
    backend::raw::DeviceMonitor(config->workerCount()),
    _config (std::move(config)),
    _virtual_input (std::move(virtual_input))
{
}

std::shared_ptr<DeviceManager> DeviceManager::make(
        const std::shared_ptr<Configuration>& config,
        const std::shared_ptr<InputDevice>& virtual_input)
{
    auto ret = std::make_shared<_DeviceManager>(config, virtual_input);
    ret->_self = ret;
    return ret;
}

std::shared_ptr<Configuration> DeviceManager::config() const
{
    return _config;
}

std::shared_ptr<InputDevice> DeviceManager::virtualInput() const
{
    return _virtual_input;
}

void DeviceManager::addDevice(std::string path)
{
    bool defaultExists = true;
    bool isReceiver = false;

    // Check if device is ignored before continuing
    {
        raw::RawDevice raw_dev(path, config()->ioTimeout(), workQueue());
        if(config()->isIgnored(raw_dev.productId())) {
            logPrintf(DEBUG, "%s: Device 0x%04x ignored.",
                  path.c_str(), raw_dev.productId());
            return;
        }
    }

    try {
        hidpp::Device device(path, hidpp::DefaultDevice,
                             config()->ioTimeout(), workQueue());
        isReceiver = device.version() == std::make_tuple(1, 0);
    } catch(hidpp10::Error &e) {
        if(e.code() != hidpp10::Error::UnknownDevice)
            throw;
    } catch(hidpp::Device::InvalidDevice &e) { // Ignore
        defaultExists = false;
    } catch(std::system_error &e) {
        logPrintf(WARN, "I/O error on %s: %s, skipping device.",
                path.c_str(), e.what());
        return;
    } catch (TimeoutError &e) {
        logPrintf(WARN, "Device %s timed out.", path.c_str());
        defaultExists = false;
    }

    if(isReceiver) {
        logPrintf(INFO, "Detected receiver at %s", path.c_str());
        auto receiver = std::make_shared<Receiver>(path, _self.lock());
        receiver->run();
        _receivers.emplace(path, receiver);
    } else {
         /* TODO: Can non-receivers only contain 1 device?
         * If the device exists, it is guaranteed to be an HID++ 2.0 device */
        if(defaultExists) {
            auto device = std::make_shared<Device>(path, hidpp::DefaultDevice,
                                                   _self.lock());
            _devices.emplace(path,  device);
        } else {
            try {
                auto device = std::make_shared<Device>(path,
                        hidpp::CordedDevice, _self.lock());
                _devices.emplace(path, device);
            } catch(hidpp10::Error &e) {
                if(e.code() != hidpp10::Error::UnknownDevice)
                    throw;
                else
                    logPrintf(WARN,
                            "HID++ 1.0 error while trying to initialize %s:"
                            "%s", path.c_str(), e.what());
            } catch(hidpp::Device::InvalidDevice &e) { // Ignore
            } catch(std::system_error &e) {
                // This error should have been thrown previously
                logPrintf(WARN, "I/O error on %s: %s", path.c_str(),
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
        logPrintf(INFO, "Receiver on %s disconnected", path.c_str());
    } else {
        auto device = _devices.find(path);
        if(device != _devices.end()) {
            _devices.erase(device);
            logPrintf(INFO, "Device on %s disconnected", path.c_str());
        }
    }
}
