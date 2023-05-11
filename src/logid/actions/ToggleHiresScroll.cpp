/*
 * Copyright 2019-2023 PixlOne
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
#include <actions/ToggleHiresScroll.h>
#include <Device.h>
#include <backend/hidpp20/features/ReprogControls.h>
#include <util/task.h>
#include <util/log.h>

using namespace logid::actions;
using namespace logid::backend;

const char* ToggleHiresScroll::interface_name = "ToggleHiresScroll";

ToggleHiresScroll::ToggleHiresScroll(
        Device* dev,
        [[maybe_unused]] const std::shared_ptr<ipcgull::node>& parent) :
        Action(dev, interface_name) {
    _hires_scroll = _device->getFeature<features::HiresScroll>("hiresscroll");
    if (!_hires_scroll)
        logPrintf(WARN, "%s:%d: HiresScroll feature not found, cannot use "
                        "ToggleHiresScroll action.",
                  _device->hidpp20().devicePath().c_str(),
                  _device->hidpp20().devicePath().c_str());
}

void ToggleHiresScroll::press() {
    _pressed = true;
    if (_hires_scroll) {
        run_task([self_weak = self<ToggleHiresScroll>()]() {
            if (auto self = self_weak.lock()) {
                auto mode = self->_hires_scroll->getMode();
                mode ^= backend::hidpp20::HiresScroll::HiRes;
                self->_hires_scroll->setMode(mode);
            }
        });
    }
}

void ToggleHiresScroll::release() {
    _pressed = false;
}

uint8_t ToggleHiresScroll::reprogFlags() const {
    return hidpp20::ReprogControls::TemporaryDiverted;
}
