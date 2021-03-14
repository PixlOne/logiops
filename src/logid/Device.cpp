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
#include "util/log.h"
#include "features/DPI.h"
#include "Device.h"
#include "Receiver.h"
#include "features/SmartShift.h"
#include "features/RemapButton.h"
#include "backend/hidpp20/features/Reset.h"
#include "features/HiresScroll.h"
#include "features/DeviceStatus.h"
#include "features/ThumbWheel.h"

using namespace logid;
using namespace logid::backend;

Device::Device(std::string path, backend::hidpp::DeviceIndex index) :
    _hidpp20 (path, index), _path (std::move(path)), _index (index),
    _config (global_config, this), _receiver (nullptr)
{
    _init();
}

Device::Device(const std::shared_ptr<backend::raw::RawDevice>& raw_device,
        hidpp::DeviceIndex index) : _hidpp20(raw_device, index), _path
        (raw_device->hidrawPath()), _index (index),
        _config (global_config, this), _receiver (nullptr)
{
    _init();
}

Device::Device(Receiver* receiver, hidpp::DeviceIndex index) : _hidpp20
    (receiver->rawReceiver(), index), _path (receiver->path()), _index (index),
        _config (global_config, this), _receiver (receiver)
{
    _init();
}

void Device::_init()
{
    logPrintf(INFO, "Device found: %s on %s:%d", name().c_str(),
            hidpp20().devicePath().c_str(), _index);

    _addFeature<features::DPI>("dpi");
    _addFeature<features::SmartShift>("smartshift");
    _addFeature<features::HiresScroll>("hiresscroll");
    _addFeature<features::RemapButton>("remapbutton");
    _addFeature<features::DeviceStatus>("devicestatus");
    _addFeature<features::ThumbWheel>("thumbwheel");

    _makeResetMechanism();
    reset();

    for(auto& feature: _features) {
        feature.second->configure();
        feature.second->listen();
    }

    _hidpp20.listen();
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
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    reset();

    for(auto& feature: _features)
        feature.second->configure();
}

void Device::reset()
{
    if(_reset_mechanism)
        (*_reset_mechanism)();
    else
        logPrintf(DEBUG, "%s:%d tried to reset, but no reset mechanism was "
                         "available.", _path.c_str(), _index);
}

DeviceConfig& Device::config()
{
    return _config;
}

hidpp20::Device& Device::hidpp20()
{
    return _hidpp20;
}

void Device::_makeResetMechanism()
{
    try {
        hidpp20::Reset reset(&_hidpp20);
        _reset_mechanism = std::make_unique<std::function<void()>>(
                [dev=&this->_hidpp20]{
                    hidpp20::Reset reset(dev);
                        reset.reset(reset.getProfile());
                });
    } catch(hidpp20::UnsupportedFeature& e) {
        // Reset unsupported, ignore.
    }
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

libconfig::Setting& DeviceConfig::getSetting(const std::string& path)
{
    return _config->getSetting(_root_setting + '/' + path);
}
