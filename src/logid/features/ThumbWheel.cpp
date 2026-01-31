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

#include <features/ThumbWheel.h>
#include <actions/gesture/AxisGesture.h>
#include <Device.h>
#include <util/log.h>
#include <util/task.h>
#include <ipc_defs.h>

using namespace logid::features;
using namespace logid::backend;
using namespace logid;

#define FLAG_STR(b) (_wheel_info.capabilities & _thumb_wheel->b ? "YES" : "NO")

namespace {
    std::shared_ptr<actions::Action> _genAction(
            Device* dev, std::optional<config::BasicAction>& conf,
            const std::shared_ptr<ipcgull::node>& parent) {
        if (conf.has_value()) {
            try {
                return actions::Action::makeAction(dev, conf.value(), parent);
            } catch (actions::InvalidAction& e) {
                logPrintf(WARN, "Mapping thumb wheel to invalid action");
            }
        }

        return nullptr;
    }

    std::shared_ptr<actions::Gesture> _genGesture(
            Device* dev, std::optional<config::Gesture>& conf,
            const std::shared_ptr<ipcgull::node>& parent, const std::string& direction) {
        if (conf.has_value()) {
            try {
                auto result = actions::Gesture::makeGesture(dev, conf.value(),
                                                            parent->make_child(direction));
                if (!result->wheelCompatibility()) {
                    logPrintf(WARN, "Mapping thumb wheel to incompatible gesture");
                    return nullptr;
                } else {
                    return result;
                }
            } catch (actions::InvalidAction& e) {
                logPrintf(WARN, "Mapping thumb wheel to invalid gesture");
            }
        }

        return nullptr;
    }
}

ThumbWheel::ThumbWheel(Device* dev) : DeviceFeature(dev), _wheel_info(),
                                      _node(dev->ipcNode()->make_child("thumbwheel")),
                                      _left_node(_node->make_child("left")),
                                      _right_node(_node->make_child("right")),
                                      _proxy_node(_node->make_child("proxy")),
                                      _tap_node(_node->make_child("tap")),
                                      _touch_node(_node->make_child("touch")),
                                      _config(dev->activeProfile().thumbwheel) {

    try {
        _thumb_wheel = std::make_shared<hidpp20::ThumbWheel>(&dev->hidpp20());
    } catch (hidpp20::UnsupportedFeature& e) {
        throw UnsupportedFeature();
    }

    _makeConfig();

    _wheel_info = _thumb_wheel->getInfo();

    logPrintf(DEBUG, "Thumb wheel detected (0x2150), capabilities:");
    logPrintf(DEBUG, "timestamp | touch | proximity | single tap");
    logPrintf(DEBUG, "%-9s | %-5s | %-9s | %-10s", FLAG_STR(Timestamp),
              FLAG_STR(Touch), FLAG_STR(Proxy), FLAG_STR(SingleTap));
    logPrintf(DEBUG, "Thumb wheel resolution: native (%d), diverted (%d)",
              _wheel_info.nativeRes, _wheel_info.divertedRes);

    if (_left_gesture) {
        _fixGesture(_left_gesture);
    }

    if (_right_gesture) {
        _fixGesture(_right_gesture);
    }

    _ipc_interface = dev->ipcNode()->make_interface<IPC>(this);
}

void ThumbWheel::_makeConfig() {
    if (_config.get().has_value()) {
        auto& conf = _config.get().value();
        _left_gesture = _genGesture(_device, conf.left, _left_node, "left");
        _right_gesture = _genGesture(_device, conf.right, _right_node, "right");
        _touch_action = _genAction(_device, conf.touch, _touch_node);
        _tap_action = _genAction(_device, conf.tap, _tap_node);
        _proxy_action = _genAction(_device, conf.proxy, _proxy_node);
    }
}

void ThumbWheel::configure() {
    std::shared_lock lock(_config_mutex);
    auto& config = _config.get();
    if (config.has_value()) {
        const auto& value = config.value();
        _thumb_wheel->setStatus(value.divert.value_or(false),
                                value.invert.value_or(false));
    }
}

void ThumbWheel::listen() {
    if (_ev_handler.empty()) {
        _ev_handler = _device->hidpp20().addEventHandler(
                {[index = _thumb_wheel->featureIndex()]
                         (const hidpp::Report& report) -> bool {
                    return (report.feature() == index) &&
                           (report.function() == hidpp20::ThumbWheel::Event);
                },
                 [self_weak = self<ThumbWheel>()](const hidpp::Report& report) -> void {
                     if (auto self = self_weak.lock())
                        self->_handleEvent(self->_thumb_wheel->thumbwheelEvent(report));
                 }
                });
    }
}

void ThumbWheel::setProfile(config::Profile& profile) {
    std::unique_lock lock(_config_mutex);
    _config = profile.thumbwheel;
    _left_gesture.reset();
    _right_gesture.reset();
    _touch_action.reset();
    _tap_action.reset();
    _proxy_action.reset();
    _makeConfig();
}

void ThumbWheel::_handleEvent(hidpp20::ThumbWheel::ThumbwheelEvent event) {
    std::shared_lock lock(_config_mutex);
    if (event.flags & hidpp20::ThumbWheel::SingleTap) {
        _touch_pending = false; // Cancel pending touch on tap
        auto action = _tap_action;
        if (action) {
            action->press();
            action->release();
        }
    }

    if ((bool) (event.flags & hidpp20::ThumbWheel::Proxy) != _last_proxy) {
        _last_proxy = !_last_proxy;
        if (_proxy_action) {
            if (_last_proxy)
                _proxy_action->press();
            else
                _proxy_action->release();
        }
    }

    if ((bool) (event.flags & hidpp20::ThumbWheel::Touch) != _last_touch) {
        _last_touch = !_last_touch;
        if (_touch_action) {
            if (_last_touch) {
                // Touch started: start timer
                _touch_pending = true;
                auto now = std::chrono::system_clock::now();
                _touch_time = now;
                
                logid::run_task_after([this, now]() {
                    // Only press if still pending AND it's the same touch session
                    if (_touch_pending.load() && _touch_time == now) {
                        std::shared_lock lock(_config_mutex);
                        // Re-check after acquiring lock
                        if (_touch_pending.load() && _touch_time == now) {
                            if (_touch_action)
                                _touch_action->press();
                            _touch_active = true;
                            _touch_pending = false;
                        }
                    }
                }, std::chrono::milliseconds(TOUCH_DELAY_MS));
            } else {
                // Touch stopped: ALWAYS release
                _touch_pending = false;
                if (_touch_active.exchange(false)) {
                    std::shared_lock lock(_config_mutex);
                    if (_touch_action)
                        _touch_action->release();
                }
            }
        }
    }

    if (event.rotationStatus != hidpp20::ThumbWheel::Inactive) {
        // Make right positive unless inverted
        event.rotation *= _wheel_info.defaultDirection;


        if (event.rotation) {
            _touch_pending = false; // Cancel pending touch on movement
            std::shared_ptr<actions::Gesture> scroll_action;

            if (event.rotation > 0)
                scroll_action = _right_gesture;
            else
                scroll_action = _left_gesture;

            if (scroll_action) {
                scroll_action->move(event.rotation);
            }
        }

        if (event.rotationStatus == hidpp20::ThumbWheel::Stop) {
            if (_right_gesture)
                _right_gesture->release(false);
            if (_left_gesture)
                _left_gesture->release(false);
        }
    }
}

void ThumbWheel::_fixGesture(const std::shared_ptr<actions::Gesture>& gesture) const {
    try {
        auto axis = std::dynamic_pointer_cast<actions::AxisGesture>(gesture);
        // TODO: How do hires multipliers work on 0x2150 thumbwheels?
        if (axis)
            axis->setHiresMultiplier(_wheel_info.divertedRes);
    } catch (std::bad_cast& e) {}

    if (gesture) {
        // gesture->press(true); // Removed to prevent setting initial axis to +50
    }
}

ThumbWheel::IPC::IPC(ThumbWheel* parent) : ipcgull::interface(
        SERVICE_ROOT_NAME ".ThumbWheel", {
                {"GetConfig", {this, &IPC::getConfig, {"divert", "invert"}}},
                {"SetDivert", {this, &IPC::setDivert, {"divert"}}},
                {"SetInvert", {this, &IPC::setInvert, {"invert"}}},
                {"SetLeft",   {this, &IPC::setLeft,   {"type"}}},
                {"SetRight",  {this, &IPC::setRight,  {"type"}}},
                {"SetProxy",  {this, &IPC::setProxy,  {"type"}}},
                {"SetTap",    {this, &IPC::setTap,    {"type"}}},
                {"SetTouch",  {this, &IPC::setTouch,  {"type"}}},
        }, {}, {}), _parent(*parent) {
}

config::ThumbWheel& ThumbWheel::IPC::_parentConfig() {
    auto& config = _parent._config.get();
    if (!config.has_value()) {
        config.emplace();
    }

    return config.value();
}

std::tuple<bool, bool> ThumbWheel::IPC::getConfig() const {
    std::shared_lock lock(_parent._config_mutex);

    auto& config = _parent._config.get();
    if (!config.has_value()) {
        return {false, false};
    }

    return {config.value().divert.value_or(false),
            config.value().invert.value_or(false)};
}

void ThumbWheel::IPC::setDivert(bool divert) {
    std::unique_lock lock(_parent._config_mutex);

    auto& config = _parentConfig();
    config.divert = divert;

    _parent._thumb_wheel->setStatus(divert, config.invert.value_or(false));
}

void ThumbWheel::IPC::setInvert(bool invert) {
    std::unique_lock lock(_parent._config_mutex);

    auto& config = _parentConfig();
    config.invert = invert;

    _parent._thumb_wheel->setStatus(config.divert.value_or(false), invert);
}

void ThumbWheel::IPC::setLeft(const std::string& type) {
    std::unique_lock lock(_parent._config_mutex);

    auto& config = _parentConfig();

    _parent._left_gesture.reset();
    if (!config.left.has_value()) {
        config.left = config::NoGesture();
    }
    _parent._left_gesture = actions::Gesture::makeGesture(
            _parent._device, type, config.left.value(), _parent._left_node);
    if (!_parent._left_gesture->wheelCompatibility()) {
        _parent._left_gesture.reset();
        config.left.reset();

        throw std::invalid_argument("incompatible gesture");
    } else {
        _parent._fixGesture(_parent._left_gesture);
    }
}

void ThumbWheel::IPC::setRight(const std::string& type) {
    std::unique_lock lock(_parent._config_mutex);

    auto& config = _parentConfig();

    if (!config.right.has_value()) {
        config.right = config::NoGesture();
    }
    _parent._right_gesture = actions::Gesture::makeGesture(
            _parent._device, type, config.right.value(), _parent._right_node);
    if (!_parent._right_gesture->wheelCompatibility()) {
        _parent._right_gesture.reset();
        config.right.reset();

        throw std::invalid_argument("incompatible gesture");
    } else {
        _parent._fixGesture(_parent._right_gesture);
    }
}

void ThumbWheel::IPC::setProxy(const std::string& type) {
    std::unique_lock lock(_parent._config_mutex);

    auto& config = _parentConfig();

    _parent._proxy_action = actions::Action::makeAction(
            _parent._device, type, config.proxy, _parent._proxy_node);
}


void ThumbWheel::IPC::setTap(const std::string& type) {
    std::unique_lock lock(_parent._config_mutex);

    auto& config = _parentConfig();

    _parent._tap_action = actions::Action::makeAction(
            _parent._device, type, config.tap, _parent._tap_node);
}


void ThumbWheel::IPC::setTouch(const std::string& type) {
    std::unique_lock lock(_parent._config_mutex);

    auto& config = _parentConfig();

    _parent._touch_action = actions::Action::makeAction(
            _parent._device, type, config.touch, _parent._touch_node);
}
