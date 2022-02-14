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

const char* ChangeDPI::interface_name = "ChangeDPI";

ChangeDPI::ChangeDPI(
        Device *device, config::ChangeDPI& config,
        const std::shared_ptr<ipcgull::node>& parent) :
    Action(device, interface_name), _config (config)
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
    if(_dpi && _config.inc.has_value()) {
        spawn_task(
        [this]{
            try {
                uint16_t last_dpi = _dpi->getDPI(_config.sensor.value_or(0));
                _dpi->setDPI(last_dpi + _config.inc.value(),
                             _config.sensor.value_or(0));
            } catch (backend::hidpp20::Error& e) {
                if(e.code() == backend::hidpp20::Error::InvalidArgument)
                    logPrintf(WARN, "%s:%d: Could not get/set DPI for sensor "
                                    "%d",
                              _device->hidpp20().devicePath().c_str(),
                              _device->hidpp20().deviceIndex(),
                              _config.sensor.value_or(0));
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
