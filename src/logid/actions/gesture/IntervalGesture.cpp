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
#include "IntervalGesture.h"
#include "../../util/log.h"
#include "../../Configuration.h"

using namespace logid::actions;

IntervalGesture::IntervalGesture(Device *device, config::IntervalGesture& config,
                                 const std::shared_ptr<ipcgull::node>& parent,
                                 const std::string& direction) :
    Gesture (device, parent, direction), _config (config)
{
    if(config.action) {
        try {
            _action = Action::makeAction(device, config.action.value(), _node);
        } catch(InvalidAction& e) {
            logPrintf(WARN, "Mapping gesture to invalid action");
        }
    }
}

void IntervalGesture::press(bool init_threshold)
{
    _axis = init_threshold ?
            _config.threshold.value_or(defaults::gesture_threshold) : 0;
    _interval_pass_count = 0;
}

void IntervalGesture::release(bool primary)
{
    // Do nothing
    (void)primary; // Suppress unused warning
}

void IntervalGesture::move(int16_t axis)
{
    const auto threshold =
            _config.threshold.value_or(defaults::gesture_threshold);
    _axis += axis;
    if(_axis < threshold)
        return;

    int16_t new_interval_count = (_axis - threshold)/
            _config.interval;
    if(new_interval_count > _interval_pass_count) {
        if(_action) {
            _action->press();
            _action->release();
        }
    }
    _interval_pass_count = new_interval_count;
}

bool IntervalGesture::wheelCompatibility() const
{
    return true;
}

bool IntervalGesture::metThreshold() const
{
    return _axis >= _config.threshold.value_or(defaults::gesture_threshold);;
}
