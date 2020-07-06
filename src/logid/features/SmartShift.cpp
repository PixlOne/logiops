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
#include "SmartShift.h"
#include "../Device.h"
#include "../util/log.h"

using namespace logid::features;
using namespace logid::backend;

SmartShift::SmartShift(Device* device) : DeviceFeature(device), _config
    (device), _smartshift(&device->hidpp20())
{
}

void SmartShift::configure()
{
    _smartshift.setStatus(_config.getSettings());
}

void SmartShift::listen()
{
}

SmartShift::Config::Config(Device *dev) : DeviceFeature::Config(dev), _status()
{
    try {
        auto& config_root = dev->config().getSetting("smartshift");
        if(!config_root.isGroup()) {
            logPrintf(WARN, "Line %d: smartshift must be an object",
                    config_root.getSourceLine());
            return;
        }
        _status.setActive = config_root.lookupValue("on", _status.active);
        int tmp;
        _status.setAutoDisengage = config_root.lookupValue("threshold", tmp);
        if(_status.setAutoDisengage)
            _status.autoDisengage = tmp;
        _status.setDefaultAutoDisengage = config_root.lookupValue
                ("default_threshold", tmp);
        if(_status.setDefaultAutoDisengage)
            _status.defaultAutoDisengage = tmp;
    } catch(libconfig::SettingNotFoundException& e) {
        // SmartShift not configured, use default
    }
}

hidpp20::SmartShift::SmartshiftStatus SmartShift::Config::getSettings()
{
    return _status;
}