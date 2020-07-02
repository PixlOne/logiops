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

#ifndef LOGID_DEVICE_H
#define LOGID_DEVICE_H

#include "backend/hidpp/defs.h"
#include "backend/hidpp20/Device.h"
#include "backend/hidpp20/Feature.h"
#include "features/DeviceFeature.h"
#include "Configuration.h"
#include "util/log.h"

namespace logid
{
    class Device;

    class DeviceConfig
    {
    public:
        DeviceConfig(const std::shared_ptr<Configuration>& config, Device*
        device);
        libconfig::Setting& getSetting(std::string path);
    private:
        Device* _device;
        std::string _root_setting;
        std::shared_ptr<Configuration> _config;
    };

    /* TODO: Implement HID++ 1.0 support
     * Currently, the logid::Device class has a hardcoded requirement
     * for an HID++ 2.0 device.
     */
    class Device
    {
    public:
        Device(std::string path, backend::hidpp::DeviceIndex index);
        Device(const std::shared_ptr<backend::raw::RawDevice>& raw_device,
                backend::hidpp::DeviceIndex index);

        std::string name();
        uint16_t pid();

        DeviceConfig& config();
        backend::hidpp20::Device& hidpp20();

        void wakeup();
        void sleep();

        template<typename T>
        std::shared_ptr<T> getFeature(std::string name) {
            auto it = _features.find(name);
            if(it == _features.end())
                return nullptr;
            try {
                return std::dynamic_pointer_cast<std::shared_ptr<T>>
                    (it->second);
            } catch(std::bad_cast& e) {
                logPrintf(ERROR, "bad_cast while getting device feature %s: "
                                 "%s", name.c_str(), e.what());
                return nullptr;
            }
        }

    private:
        void _init();

        /* Adds a feature without calling an error if unsupported */
        template<typename T>
        void _addFeature(std::string name)
        {
            try {
                _features.emplace(name, std::make_shared<T>(this));
            } catch (backend::hidpp20::UnsupportedFeature& e) {
            }
        }

        backend::hidpp20::Device _hidpp20;
        std::string _path;
        backend::hidpp::DeviceIndex _index;
        std::map<std::string, std::shared_ptr<features::DeviceFeature>>
            _features;
        DeviceConfig _config;
    };
}

#endif //LOGID_DEVICE_H
