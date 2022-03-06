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

#include "ThumbWheel.h"
#include "../Device.h"
#include "../actions/gesture/AxisGesture.h"

using namespace logid::features;
using namespace logid::backend;
using namespace logid;

#define FLAG_STR(b) (_wheel_info.capabilities & _thumb_wheel->b ? "YES" : \
    "NO")


std::shared_ptr<actions::Action> _genAction(
        Device* dev, std::optional<config::BasicAction>& conf,
        const std::shared_ptr<ipcgull::node>& parent)
{
    if(conf.has_value()) {
        try {
            return actions::Action::makeAction(dev, conf.value(), parent);
        } catch(actions::InvalidAction& e) {
            logPrintf(WARN, "Mapping thumb wheel to invalid action");
        }
    }

    return nullptr;
}

std::shared_ptr<actions::Gesture> _genGesture(
        Device* dev, std::optional<config::Gesture>& conf,
        const std::shared_ptr<ipcgull::node>& parent, const std::string& direction)
{
    if(conf.has_value()) {
        try {
            return actions::Gesture::makeGesture(
                    dev, conf.value(), parent->make_child(direction));
        } catch (actions::InvalidAction &e) {
            logPrintf(WARN, "Mapping thumb wheel to invalid gesture");
        }
    }

        return nullptr;
}

ThumbWheel::ThumbWheel(Device *dev) : DeviceFeature(dev), _wheel_info(),
    _node (dev->ipcNode()->make_child("thumbwheel")),
    _gesture_node (_node->make_child("scroll")),
    _proxy_node (_node->make_child("proxy")),
    _tap_node (_node->make_child("tap")),
    _touch_node (_node->make_child("touch")),
    _config (dev->activeProfile().thumbwheel)
{
    if(_config.has_value()) {
        auto& conf = _config.value();
        _left_action = _genGesture(dev, conf.left, _gesture_node, "left");
        _right_action = _genGesture(dev, conf.right, _gesture_node, "right");
        _touch_action = _genAction(dev, conf.touch, _touch_node);
        _tap_action = _genAction(dev, conf.tap, _tap_node);
        _proxy_action = _genAction(dev, conf.proxy, _proxy_node);
    }

    try {
        _thumb_wheel = std::make_shared<hidpp20::ThumbWheel>(&dev->hidpp20());
    } catch(hidpp20::UnsupportedFeature& e) {
        throw UnsupportedFeature();
    }

    _wheel_info = _thumb_wheel->getInfo();

    logPrintf(DEBUG,"Thumb wheel detected (0x2150), capabilities:");
    logPrintf(DEBUG, "timestamp | touch | proximity | single tap");
    logPrintf(DEBUG, "%-9s | %-5s | %-9s | %-10s", FLAG_STR(Timestamp),
              FLAG_STR(Touch), FLAG_STR(Proxy), FLAG_STR(SingleTap));
    logPrintf(DEBUG, "Thumb wheel resolution: native (%d), diverted (%d)",
              _wheel_info.nativeRes, _wheel_info.divertedRes);

    if(_left_action) {
        try {
            auto left_axis = std::dynamic_pointer_cast<actions::AxisGesture>(
                    _left_action);
            // TODO: How do hires multipliers work on 0x2150 thumbwheels?
            if(left_axis)
                left_axis->setHiresMultiplier(_wheel_info.divertedRes);
        } catch(std::bad_cast& e) { }

        _left_action->press(true);
    }

    if(_right_action) {
        try {
            auto right_axis = std::dynamic_pointer_cast<actions::AxisGesture>(
                    _right_action);
            if(right_axis)
                right_axis->setHiresMultiplier(_wheel_info.divertedRes);
        } catch(std::bad_cast& e) { }

        _right_action->press(true);
    }
}

ThumbWheel::~ThumbWheel()
{
    if(_ev_handler.has_value())
        _device->hidpp20().removeEventHandler(_ev_handler.value());
}

void ThumbWheel::configure()
{
    if(_config.has_value()) {
        const auto& config = _config.value();
        _thumb_wheel->setStatus(config.divert.value_or(false),
                                config.invert.value_or(false));
    }
}

void ThumbWheel::listen()
{
    if(!_ev_handler.has_value()) {
        _ev_handler = _device->hidpp20().addEventHandler({
            [index=_thumb_wheel->featureIndex()]
                    (const hidpp::Report& report)->bool {
                return (report.feature() == index) &&
                (report.function() == hidpp20::ThumbWheel::Event);
            },
            [this](const hidpp::Report& report)->void {
                _handleEvent(_thumb_wheel->thumbwheelEvent(report));
            }
        });
    }
}

void ThumbWheel::_handleEvent(hidpp20::ThumbWheel::ThumbwheelEvent event)
{
    if(event.flags & hidpp20::ThumbWheel::SingleTap) {
        auto action = _tap_action;
        if(action) {
            action->press();
            action->release();
        }
    }

    if((bool)(event.flags & hidpp20::ThumbWheel::Proxy) != _last_proxy) {
        _last_proxy = !_last_proxy;
        if(_proxy_action) {
            if(_last_proxy)
                _proxy_action->press();
            else
                _proxy_action->release();
        }
    }

    if((bool)(event.flags & hidpp20::ThumbWheel::Touch) != _last_touch) {
        _last_touch = !_last_touch;
        if(_touch_action) {
            if(_last_touch)
                _touch_action->press();
            else
                _touch_action->release();
        }
    }

    if(event.rotationStatus != hidpp20::ThumbWheel::Inactive) {
        // Make right positive unless inverted
        event.rotation *= _wheel_info.defaultDirection;

        if(event.rotationStatus == hidpp20::ThumbWheel::Start) {
            if(_right_action)
               _right_action->press(true);
            if(_left_action)
                _left_action->press(true);
            _last_direction = 0;
        }

        if(event.rotation) {
            int8_t direction = event.rotation > 0 ? 1 : -1;
            std::shared_ptr<actions::Gesture> scroll_action;

            if(direction > 0)
                scroll_action = _right_action;
            else
                scroll_action = _left_action;

            if(scroll_action) {
                scroll_action->press(true);
                scroll_action->move(direction * event.rotation);
            }

            _last_direction = direction;
        }

        if(event.rotationStatus == hidpp20::ThumbWheel::Stop) {
            if(_right_action)
                _right_action->release();
            if(_left_action)
                _left_action->release();
        }
    }
}
