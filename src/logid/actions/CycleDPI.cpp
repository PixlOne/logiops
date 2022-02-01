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
#include "CycleDPI.h"
#include "../Device.h"
#include "../util/task.h"
#include "../util/log.h"
#include "../backend/hidpp20/Error.h"
#include "../backend/hidpp20/features/ReprogControls.h"

using namespace logid::actions;
using namespace libconfig;

CycleDPI::CycleDPI(Device* device, libconfig::Setting& setting) :
    Action (device), _config (device, setting)
{
    _dpi = _device->getFeature<features::DPI>("dpi");
    if(!_dpi)
        logPrintf(WARN, "%s:%d: DPI feature not found, cannot use "
                        "CycleDPI action.",
                  _device->hidpp20().devicePath().c_str(),
                  _device->hidpp20().deviceIndex());
}

void CycleDPI::press()
{
    _pressed = true;
    if(_dpi && !_config.empty()) {
        task::spawn([this](){
            uint16_t dpi = _config.nextDPI();
            try {
                _dpi->setDPI(dpi, _config.sensor());
            } catch (backend::hidpp20::Error& e) {
                if(e.code() == backend::hidpp20::Error::InvalidArgument)
                    logPrintf(WARN, "%s:%d: Could not set DPI to %d for "
                        "sensor %d", _device->hidpp20().devicePath().c_str(),
                        _device->hidpp20().deviceIndex(), dpi,
                        _config.sensor());
                else
                    throw e;
            }
        });
    }
}

void CycleDPI::release()
{
    _pressed = false;
}

uint8_t CycleDPI::reprogFlags() const
{
    return backend::hidpp20::ReprogControls::TemporaryDiverted;
}

CycleDPI::Config::Config(Device *device, libconfig::Setting &config) :
    Action::Config(device),  _current_index (0), _sensor (0)
{
    if(!config.isGroup()) {
        logPrintf(WARN, "Line %d: action must be an object, skipping.",
                  config.getSourceLine());
        return;
    }

    try {
        auto& sensor = config["sensor"];
        if(sensor.getType() != Setting::TypeInt)
            logPrintf(WARN, "Line %d: sensor must be an integer",
                    sensor.getSourceLine());
        _sensor = (int)sensor;
    } catch(libconfig::SettingNotFoundException& e) {
        // Ignore
    }

    try {
        auto& dpis = config["dpis"];
        if(!dpis.isList() && !dpis.isArray()) {
            logPrintf(WARN, "Line %d: dpis must be a list or array, skipping.",
                    dpis.getSourceLine());
            return;
        }

        int dpi_count = dpis.getLength();
        for(int i = 0; i < dpi_count; i++) {
            if(dpis[i].getType() != Setting::TypeInt) {
                logPrintf(WARN, "Line %d: dpis must be integers, skipping.",
                        dpis[i].getSourceLine());
                if(dpis.isList())
                    continue;
                else
                    break;
            }

            _dpis.push_back((int)(dpis[i]));
        }

    } catch (libconfig::SettingNotFoundException& e) {
        logPrintf(WARN, "Line %d: dpis is a required field, skipping.",
                config.getSourceLine());
    }
}

uint16_t CycleDPI::Config::nextDPI()
{
    uint16_t dpi = _dpis[_current_index++];
    if(_current_index >= _dpis.size())
        _current_index = 0;
    return dpi;
}

bool CycleDPI::Config::empty() const
{
    return _dpis.empty();
}

uint8_t CycleDPI::Config::sensor() const
{
    return _sensor;
}
