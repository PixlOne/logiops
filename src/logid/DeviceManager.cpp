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

#include "DeviceManager.h"
#include "Receiver.h"
#include "util/log.h"
#include "backend/hidpp10/Error.h"
#include "backend/dj/Receiver.h"
#include "backend/Error.h"

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
        logPrintf(WARN, "I/O error on %s: %s, skipping device.",
                path.c_str(), e.what());
        return;
    } catch (TimeoutError &e) {
        logPrintf(WARN, "Device %s timed out.", path.c_str());
        defaultExists = false;
    }

    if(isReceiver) {
        logPrintf(INFO, "Detected receiver at %s", path.c_str());
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
        if(device != _devices.find(path)) {
            _devices.erase(device);
            logPrintf(INFO, "Device on %s disconnected", path.c_str());
        }
    }
}
