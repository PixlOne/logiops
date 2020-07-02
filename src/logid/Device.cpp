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

#include "util/log.h"
#include "features/DPI.h"
#include "Device.h"
#include "features/SmartShift.h"

using namespace logid;
using namespace logid::backend;

Device::Device(std::string path, backend::hidpp::DeviceIndex index) :
    _hidpp20 (path, index), _path (std::move(path)), _index (index),
    _config (global_config, this)
{
    _init();
}

Device::Device(const std::shared_ptr<backend::raw::RawDevice>& raw_device,
        hidpp::DeviceIndex index) : _hidpp20(raw_device, index), _path
        (raw_device->hidrawPath()), _index (index),
        _config (global_config, this)
{
    _init();
}

void Device::_init()
{
    _addFeature<features::DPI>();
    _addFeature<features::SmartShift>();

    for(auto& feature: _features)
        feature->configure();
}

std::string Device::name()
{
    return _hidpp20.name();
}

uint16_t Device::pid()
{
    return _hidpp20.pid();
}

void Device::sleep()
{
    logPrintf(INFO, "%s:%d fell asleep.", _path.c_str(), _index);
}

void Device::wakeup()
{
    logPrintf(INFO, "%s:%d woke up.", _path.c_str(), _index);
}

DeviceConfig& Device::config()
{
    return _config;
}

hidpp20::Device& Device::hidpp20()
{
    return _hidpp20;
}

DeviceConfig::DeviceConfig(const std::shared_ptr<Configuration>& config, Device*
    device) : _device (device), _config (config)
{
    try {
        _root_setting = config->getDevice(device->name());
    } catch(Configuration::DeviceNotFound& e) {
        logPrintf(INFO, "Device %s not configured, using default config.",
                device->name().c_str());
    }
}

libconfig::Setting& DeviceConfig::getSetting(std::string path)
{
    return _config->getSetting(_root_setting + '/' + path);
}
