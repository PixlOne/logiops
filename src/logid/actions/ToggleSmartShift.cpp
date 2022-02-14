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
#include "ToggleSmartShift.h"
#include "../Device.h"
#include "../backend/hidpp20/features/ReprogControls.h"
#include "../util/task.h"

using namespace logid::actions;
using namespace logid::backend;

const char* ToggleSmartShift::interface_name = "ToggleSmartShift";

ToggleSmartShift::ToggleSmartShift(
        Device *dev,
        [[maybe_unused]] const std::shared_ptr<ipcgull::node>& parent) :
        Action (dev, interface_name)
{
    _smartshift = _device->getFeature<features::SmartShift>("smartshift");
    if(!_smartshift)
        logPrintf(WARN, "%s:%d: SmartShift feature not found, cannot use "
                        "ToggleSmartShift action.",
                        _device->hidpp20().devicePath().c_str(),
                        _device->hidpp20().deviceIndex());
}

void ToggleSmartShift::press()
{
    _pressed = true;
    if(_smartshift) {
        spawn_task(
        [ss=this->_smartshift](){
            auto status = ss->getStatus();
            status.setActive = true;
            status.active = !status.active;
            ss->setStatus(status);
        });
    }
}

void ToggleSmartShift::release()
{
    _pressed = false;
}

uint8_t ToggleSmartShift::reprogFlags() const
{
    return hidpp20::ReprogControls::TemporaryDiverted;
}
