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

SmartShift::SmartShift(Device* device) : DeviceFeature(device),
    _config (device->activeProfile().smartshift)
{
    try {
        _smartshift = std::make_shared<hidpp20::SmartShift>(&device->hidpp20());
    } catch (hidpp20::UnsupportedFeature& e) {
        throw UnsupportedFeature();
    }
}

void SmartShift::configure()
{
    if(_config.has_value()) {
        const auto& conf = _config.value();
        hidpp20::SmartShift::SmartshiftStatus settings {};
        settings.setActive = conf.on.has_value();
        if(settings.setActive)
            settings.active = conf.on.value();
        settings.setAutoDisengage = conf.threshold.has_value();
        if(settings.setAutoDisengage)
            settings.autoDisengage = conf.threshold.value();

        _smartshift->setStatus(settings);
    }
}

void SmartShift::listen()
{
}

hidpp20::SmartShift::SmartshiftStatus SmartShift::getStatus()
{
    return _smartshift->getStatus();
}

void SmartShift::setStatus(backend::hidpp20::SmartShift::SmartshiftStatus
    status)
{
    _smartshift->setStatus(status);
}
