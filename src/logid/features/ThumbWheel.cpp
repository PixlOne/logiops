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

#define SCROLL_EVENTHANDLER_NAME "THUMB_WHEEL"

ThumbWheel::ThumbWheel(Device *dev) : DeviceFeature(dev), _wheel_info(),
    _config(dev)
{
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

    if(_config.leftAction()) {
        try {
            auto left_axis = std::dynamic_pointer_cast<actions::AxisGesture>(
                    _config.leftAction());
            // TODO: How do hires multipliers work on 0x2150 thumbwheels?
            if(left_axis)
                left_axis->setHiresMultiplier(_wheel_info.divertedRes);
        } catch(std::bad_cast& e) { }

        _config.leftAction()->press(true);
    }

    if(_config.rightAction()) {
        try {
            auto right_axis = std::dynamic_pointer_cast<actions::AxisGesture>(
                    _config.rightAction());
            if(right_axis)
                right_axis->setHiresMultiplier(_wheel_info.divertedRes);
        } catch(std::bad_cast& e) { }

        _config.rightAction()->press(true);
    }
}

void ThumbWheel::configure()
{
    _thumb_wheel->setStatus(_config.divert(), _config.invert());
}

void ThumbWheel::listen()
{
    if(_device->hidpp20().eventHandlers().find(SCROLL_EVENTHANDLER_NAME) ==
       _device->hidpp20().eventHandlers().end()) {
        auto handler = std::make_shared<hidpp::EventHandler>();
        handler->condition = [index=_thumb_wheel->featureIndex()]
                (hidpp::Report& report)->bool {
            return (report.feature() == index) && (report.function() ==
                hidpp20::ThumbWheel::Event);
        };

        handler->callback = [this](hidpp::Report& report)->void {
            this->_handleEvent(_thumb_wheel->thumbwheelEvent(report));
        };

        _device->hidpp20().addEventHandler(SCROLL_EVENTHANDLER_NAME, handler);
    }
}

void ThumbWheel::_handleEvent(hidpp20::ThumbWheel::ThumbwheelEvent event)
{
    if(event.flags & hidpp20::ThumbWheel::SingleTap) {
        auto action = _config.tapAction();
        if(action) {
            action->press();
            action->release();
        }
    }

    if((bool)(event.flags & hidpp20::ThumbWheel::Proxy) != _last_proxy) {
        _last_proxy = !_last_proxy;
        auto action = _config.proxyAction();
        if(action) {
            if(_last_proxy)
                action->press();
            else
                action->release();
        }
    }

    if((bool)(event.flags & hidpp20::ThumbWheel::Touch) != _last_touch) {
        _last_touch = !_last_touch;
        auto action = _config.touchAction();
        if(action) {
            if(_last_touch)
                action->press();
            else
                action->release();
        }
    }

    if(event.rotationStatus != hidpp20::ThumbWheel::Inactive) {
        // Make right positive unless inverted
        event.rotation *= _wheel_info.defaultDirection;

        if(event.rotationStatus == hidpp20::ThumbWheel::Start) {
            if(_config.rightAction())
               _config.rightAction()->press(true);
            if(_config.leftAction())
                _config.leftAction()->press(true);
            _last_direction = 0;
        }

        if(event.rotation) {
            int8_t direction = event.rotation > 0 ? 1 : -1;
            std::shared_ptr<actions::Gesture> scroll_action;

            if(direction > 0)
                scroll_action = _config.rightAction();
            else
                scroll_action = _config.leftAction();

            if(scroll_action) {
                scroll_action->press(true);
                scroll_action->move(direction * event.rotation);
            }

            _last_direction = direction;
        }

        if(event.rotationStatus == hidpp20::ThumbWheel::Stop) {
            if(_config.rightAction())
                _config.rightAction()->release();
            if(_config.leftAction())
                _config.leftAction()->release();
        }
    }
}

ThumbWheel::Config::Config(Device* dev) : DeviceFeature::Config(dev)
{
    try {
        auto& config_root = dev->config().getSetting("thumbwheel");
        if(!config_root.isGroup()) {
            logPrintf(WARN, "Line %d: thumbwheel must be a group",
                      config_root.getSourceLine());
            return;
        }

        try {
            auto& divert = config_root["divert"];
            if(divert.getType() == libconfig::Setting::TypeBoolean) {
                _divert = divert;
            } else {
                logPrintf(WARN, "Line %d: divert must be a boolean",
                          divert.getSourceLine());
            }
        } catch(libconfig::SettingNotFoundException& e) { }

        try {
            auto& invert = config_root["invert"];
            if(invert.getType() == libconfig::Setting::TypeBoolean) {
                _invert = invert;
            } else {
                logPrintf(WARN, "Line %d: invert must be a boolean, ignoring.",
                          invert.getSourceLine());
            }
        } catch(libconfig::SettingNotFoundException& e) { }

        if(_divert) {
            _left_action = _genGesture(dev, config_root, "left");
            if(!_left_action)
                logPrintf(WARN, "Line %d: divert is true but no left action "
                                "was set", config_root.getSourceLine());

            _right_action = _genGesture(dev, config_root, "right");
            if(!_right_action)
                logPrintf(WARN, "Line %d: divert is true but no right action "
                                "was set", config_root.getSourceLine());
        }

        _proxy_action = _genAction(dev, config_root, "proxy");
        _tap_action = _genAction(dev, config_root, "tap");
        _touch_action = _genAction(dev, config_root, "touch");
    } catch(libconfig::SettingNotFoundException& e) {
        // ThumbWheel not configured, use default
    }
}

std::shared_ptr<actions::Action> ThumbWheel::Config::_genAction(Device* dev,
        libconfig::Setting& config_root, const std::string& name)
{
    try {
        auto& a_group = config_root[name.c_str()];
        try {
            return actions::Action::makeAction(dev, a_group);
        } catch(actions::InvalidAction& e) {
            logPrintf(WARN, "Line %d: Invalid action",
                      a_group.getSourceLine());
        }
    } catch(libconfig::SettingNotFoundException& e) {

    }
    return nullptr;
}

std::shared_ptr<actions::Gesture> ThumbWheel::Config::_genGesture(Device* dev,
        libconfig::Setting& config_root, const std::string& name)
{
    try {
        auto& g_group = config_root[name.c_str()];
        try {
            auto g = actions::Gesture::makeGesture(dev, g_group);
            if(g->wheelCompatibility()) {
                return g;
            } else {
                logPrintf(WARN, "Line %d: This gesture cannot be used"
                                " as a scroll action.",
                                g_group.getSourceLine());
            }
        } catch(actions::InvalidGesture& e) {
            logPrintf(WARN, "Line %d: Invalid scroll action",
                      g_group.getSourceLine());
        }
    } catch(libconfig::SettingNotFoundException& e) {

    }
    return nullptr;
}

bool ThumbWheel::Config::divert() const
{
    return _divert;
}

bool ThumbWheel::Config::invert() const
{
    return _invert;
}

const std::shared_ptr<actions::Gesture>& ThumbWheel::Config::leftAction() const
{
    return _left_action;
}

const std::shared_ptr<actions::Gesture>& ThumbWheel::Config::rightAction() const
{
    return _right_action;
}

const std::shared_ptr<actions::Action>& ThumbWheel::Config::proxyAction() const
{
    return _proxy_action;
}

const std::shared_ptr<actions::Action>& ThumbWheel::Config::tapAction() const
{
    return _tap_action;
}

const std::shared_ptr<actions::Action>& ThumbWheel::Config::touchAction() const
{
    return _touch_action;
}
