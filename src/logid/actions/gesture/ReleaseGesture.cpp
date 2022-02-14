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
#include "ReleaseGesture.h"
#include "../../Configuration.h"

using namespace logid::actions;

const char* ReleaseGesture::interface_name = "OnRelease";

ReleaseGesture::ReleaseGesture(Device *device, config::ReleaseGesture& config,
                               const std::shared_ptr<ipcgull::node>& parent) :
    Gesture (device, parent, interface_name), _config (config)
{
    if(_config.action.has_value())
        _action = Action::makeAction(device, _config.action.value(), _node);
}

void ReleaseGesture::press(bool init_threshold)
{
    _axis = init_threshold ? _config.threshold.value_or(
            defaults::gesture_threshold) : 0;
}

void ReleaseGesture::release(bool primary)
{
    if(metThreshold() && primary) {
        if(_action) {
            _action->press();
            _action->release();
        }
    }
}

void ReleaseGesture::move(int16_t axis)
{
    _axis += axis;
}

bool ReleaseGesture::wheelCompatibility() const
{
    return false;
}

bool ReleaseGesture::metThreshold() const
{
    return _axis >= _config.threshold.value_or(defaults::gesture_threshold);
}