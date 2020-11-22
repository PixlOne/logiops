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
#include "ChangeDPI.h"
#include "../Device.h"
#include "../util/task.h"
#include "../util/log.h"
#include "../backend/hidpp20/Error.h"
#include "../backend/hidpp20/features/ReprogControls.h"

using namespace logid::actions;

ChangeDPI::ChangeDPI(Device *device, libconfig::Setting &setting) :
    Action(device), _config(device, setting)
{
    _dpi = _device->getFeature<features::DPI>("dpi");
    if(!_dpi)
        logPrintf(WARN, "%s:%d: DPI feature not found, cannot use "
                        "ChangeDPI action.",
                  _device->hidpp20().devicePath().c_str(),
                  _device->hidpp20().deviceIndex());
}

void ChangeDPI::press()
{
    _pressed = true;
    if(_dpi) {
        task::spawn([this]{
            try {
                uint16_t last_dpi = _dpi->getDPI(_config.sensor());
                _dpi->setDPI(last_dpi + _config.interval(), _config.sensor());
            } catch (backend::hidpp20::Error& e) {
                if(e.code() == backend::hidpp20::Error::InvalidArgument)
                    logPrintf(WARN, "%s:%d: Could not get/set DPI for sensor "
                                    "%d",
                              _device->hidpp20().devicePath().c_str(),
                              _device->hidpp20().deviceIndex(),
                              _config.sensor());
                else
                    throw e;
            }
        });
    }
}

void ChangeDPI::release()
{
    _pressed = false;
}

uint8_t ChangeDPI::reprogFlags() const
{
    return backend::hidpp20::ReprogControls::TemporaryDiverted;
}

ChangeDPI::Config::Config(Device *device, libconfig::Setting &config) :
        Action::Config(device),  _interval (0), _sensor (0)
{
    if(!config.isGroup()) {
        logPrintf(WARN, "Line %d: action must be an object, skipping.",
                  config.getSourceLine());
        return;
    }

    try {
        auto& inc = config["inc"];
        if(inc.getType() != libconfig::Setting::TypeInt)
            logPrintf(WARN, "Line %d: inc must be an integer",
                      inc.getSourceLine());
        _interval = (int)inc;
    } catch(libconfig::SettingNotFoundException& e) {
        logPrintf(WARN, "Line %d: inc is a required field, skipping.",
            config.getSourceLine());
    }

    try {
        auto& sensor = config["sensor"];
        if(sensor.getType() != libconfig::Setting::TypeInt)
            logPrintf(WARN, "Line %d: sensor must be an integer",
                      sensor.getSourceLine());
        _sensor = (int)sensor;
    } catch(libconfig::SettingNotFoundException& e) {
        // Ignore
    }
}

uint16_t ChangeDPI::Config::interval() const
{
    return _interval;
}

uint8_t ChangeDPI::Config::sensor() const
{
    return _sensor;
}
