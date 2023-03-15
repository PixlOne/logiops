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
#include "ToggleHiresScrollAndSmartShift.h"
#include "../Device.h"
#include "../util/task.h"
#include "../backend/hidpp20/features/ReprogControls.h"

using namespace logid::actions;
using namespace logid::backend;

ToggleHiresScrollAndSmartShift::ToggleHiresScrollAndSmartShift(Device *dev) : Action (dev)
{
    _hires_scroll = _device->getFeature<features::HiresScroll>("hiresscroll");
    if(!_hires_scroll)
        logPrintf(WARN, "%s:%d: HiresScroll feature not found, cannot use "
                        "ToggleHiresScroll action.",
                  _device->hidpp20().devicePath().c_str(),
                  _device->hidpp20().devicePath().c_str());
    
    _smartshift = _device->getFeature<features::SmartShift>("smartshift");
    if(!_smartshift)
        logPrintf(WARN, "%s:%d: SmartShift feature not found, cannot use "
                        "ToggleSmartShift action.",
                        _device->hidpp20().devicePath().c_str(),
                        _device->hidpp20().deviceIndex());
}

void ToggleHiresScrollAndSmartShift::press()
{
    _pressed = true;
    if(_hires_scroll)
    {
        task::spawn([hires=this->_hires_scroll](){
            auto mode = hires->getMode();
            mode ^= backend::hidpp20::HiresScroll::HiRes;
            hires->setMode(mode);
        });
    }

    if(_smartshift) {
        task::spawn([ss=this->_smartshift](){
            auto status = ss->getStatus();
            status.setActive = true;
            status.active = !status.active;
            ss->setStatus(status);
        });
    }
}

void ToggleHiresScrollAndSmartShift::release()
{
    _pressed = false;
}

uint8_t ToggleHiresScrollAndSmartShift::reprogFlags() const
{
    return hidpp20::ReprogControls::TemporaryDiverted;
}