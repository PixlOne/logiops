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
#include <actions/gesture/NullGesture.h>
#include <Configuration.h>

using namespace logid::actions;

const char* NullGesture::interface_name = "None";

NullGesture::NullGesture(Device* device,
                         config::NoGesture& config,
                         const std::shared_ptr<ipcgull::node>& parent) :
        Gesture(device, parent, interface_name), _config(config) {
}

void NullGesture::press(bool init_threshold) {
    _axis = init_threshold ? _config.threshold.value_or(
            defaults::gesture_threshold) : 0;
}

void NullGesture::release(bool primary) {
    // Do nothing
    (void) primary; // Suppress unused warning
}

void NullGesture::move(int16_t axis) {
    _axis += axis;
}

bool NullGesture::wheelCompatibility() const {
    return true;
}

bool NullGesture::metThreshold() const {
    return _axis >= _config.threshold.value_or(defaults::gesture_threshold);
}