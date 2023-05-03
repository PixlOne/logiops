/*
 * Copyright 2019-2023 PixlOne, michtere
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
#include <actions/gesture/ThresholdGesture.h>
#include <Configuration.h>
#include <util/log.h>

using namespace logid::actions;

const char* ThresholdGesture::interface_name = "OnRelease";

ThresholdGesture::ThresholdGesture(
        Device* device, config::ThresholdGesture& config,
        const std::shared_ptr<ipcgull::node>& parent) :
        Gesture(device, parent, interface_name, {
                {
                        {"GetThreshold", {this, &ThresholdGesture::getThreshold, {"threshold"}}},
                        {"SetThreshold", {this, &ThresholdGesture::setThreshold, {"threshold"}}},
                        {"SetAction", {this, &ThresholdGesture::setAction, {"type"}}}
                },
                {},
                {}
        }), _config(config) {
    if (config.action) {
        try {
            _action = Action::makeAction(device, config.action.value(), _node);
        } catch (InvalidAction& e) {
            logPrintf(WARN, "Mapping gesture to invalid action");
        }
    }
}

void ThresholdGesture::press(bool init_threshold) {
    std::shared_lock lock(_config_mutex);
    _axis = init_threshold ? (int32_t) _config.threshold.value_or(defaults::gesture_threshold) : 0;
    this->_executed = false;
}

void ThresholdGesture::release([[maybe_unused]] bool primary) {
    this->_executed = false;
}

void ThresholdGesture::move(int16_t axis) {
    _axis += axis;

    if (!this->_executed && metThreshold()) {
        if (_action) {
            _action->press();
            _action->release();
        }
        this->_executed = true;
    }
}

bool ThresholdGesture::metThreshold() const {
    std::shared_lock lock(_config_mutex);
    return _axis >= _config.threshold.value_or(defaults::gesture_threshold);
}

bool ThresholdGesture::wheelCompatibility() const {
    return false;
}

int ThresholdGesture::getThreshold() const {
    std::shared_lock lock(_config_mutex);
    return _config.threshold.value_or(0);
}

void ThresholdGesture::setThreshold(int threshold) {
    std::unique_lock lock(_config_mutex);
    if (threshold == 0)
        _config.threshold.reset();
    else
        _config.threshold = threshold;
}

void ThresholdGesture::setAction(const std::string& type) {
    std::unique_lock lock(_config_mutex);
    _action.reset();
    _action = Action::makeAction(_device, type, _config.action, _node);
}
