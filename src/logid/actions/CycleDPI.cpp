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

const char* CycleDPI::interface_name =
        "pizza.pixl.LogiOps.Action.CycleDPI";

CycleDPI::CycleDPI(Device* device, config::CycleDPI& config,
                   const std::shared_ptr<ipcgull::node>& parent) :
    Action (device), _config (config), _current_dpi (_config.dpis.begin())
{
    _dpi = _device->getFeature<features::DPI>("dpi");
    if(!_dpi)
        logPrintf(WARN, "%s:%d: DPI feature not found, cannot use "
                        "CycleDPI action.",
                  _device->hidpp20().devicePath().c_str(),
                  _device->hidpp20().deviceIndex());

    _ipc = parent->make_interface<IPC>(this);
}

void CycleDPI::press()
{
    _pressed = true;
    if(_dpi && !_config.dpis.empty()) {
        spawn_task(
        [this](){
            std::lock_guard<std::mutex> lock(_dpi_lock);
            ++_current_dpi;
            if(_current_dpi == _config.dpis.end())
                _current_dpi = _config.dpis.begin();
            try {
                _dpi->setDPI(*_current_dpi, _config.sensor.value_or(0));
            } catch (backend::hidpp20::Error& e) {
                if(e.code() == backend::hidpp20::Error::InvalidArgument)
                    logPrintf(WARN, "%s:%d: Could not set DPI to %d for "
                        "sensor %d", _device->hidpp20().devicePath().c_str(),
                        _device->hidpp20().deviceIndex(), *_current_dpi,
                        _config.sensor.value_or(0));
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

CycleDPI::IPC::IPC(CycleDPI *action) :
    ipcgull::interface(interface_name, {}, {}, {}), _action (*action)
{
}
