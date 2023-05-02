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
#include <features/HiresScroll.h>
#include <actions/gesture/AxisGesture.h>
#include <Device.h>
#include <InputDevice.h>
#include <ipc_defs.h>

using namespace logid;
using namespace logid::features;
using namespace logid::backend;

HiresScroll::HiresScroll(Device* dev) :
        DeviceFeature(dev),
        _config(dev->activeProfile().hiresscroll), _mode(0),
        _mask(0),
        _node(dev->ipcNode()->make_child("hires_scroll")),
        _up_node(_node->make_child("up")),
        _down_node(_node->make_child("down")) {

    try {
        _hires_scroll = std::make_shared<hidpp20::HiresScroll>(&dev->hidpp20());
    } catch (hidpp20::UnsupportedFeature& e) {
        throw UnsupportedFeature();
    }

    _makeConfig();

    _last_scroll = std::chrono::system_clock::now();

    _ipc_interface = dev->ipcNode()->make_interface<IPC>(this);
}

void HiresScroll::_makeConfig() {
    auto& config = _config.get();
    _mode = 0;
    _mask = 0;

    if (config.has_value()) {
        if (std::holds_alternative<bool>(config.value())) {
            config::HiresScroll conf{};
            conf.hires = std::get<bool>(config.value());
            conf.invert = false;
            conf.target = false;
            _mask |= hidpp20::HiresScroll::Mode::HiRes;
            config.value() = conf;
        }
        auto& conf = std::get<config::HiresScroll>(config.value());
        if (conf.hires.has_value()) {
            _mask |= hidpp20::HiresScroll::Mode::HiRes;
            if (conf.hires.value())
                _mode |= hidpp20::HiresScroll::Mode::HiRes;
        }
        if (conf.invert.has_value()) {
            _mask |= hidpp20::HiresScroll::Mode::Inverted;
            if (conf.invert.value())
                _mode |= hidpp20::HiresScroll::Mode::Inverted;
        }
        if (conf.target.has_value()) {
            _mask |= hidpp20::HiresScroll::Mode::Target;
            if (conf.target.value())
                _mode |= hidpp20::HiresScroll::Mode::Target;
        }

        _makeGesture(_up_gesture, conf.up, "up");
        _makeGesture(_down_gesture, conf.down, "down");
    }
}

void HiresScroll::configure() {
    std::shared_lock lock(_config_mutex);
    _configure();
}

void HiresScroll::_configure() {
    auto mode = _hires_scroll->getMode();
    mode &= ~_mask;
    mode |= (_mode & _mask);
    _hires_scroll->setMode(mode);
}

void HiresScroll::listen() {
    std::shared_lock lock(_config_mutex);
    if (_ev_handler.empty()) {
        _ev_handler = _device->hidpp20().addEventHandler(
                {[index = _hires_scroll->featureIndex()](
                        const hidpp::Report& report) -> bool {
                    return (report.feature() == index) &&
                           (report.function() == hidpp20::HiresScroll::WheelMovement);
                },
                 [self_weak = self<HiresScroll>()](const hidpp::Report& report) {
                     if (auto self = self_weak.lock())
                         self->_handleScroll(self->_hires_scroll->wheelMovementEvent(report));
                 }
                });
    }
}

void HiresScroll::setProfile(config::Profile& profile) {
    std::unique_lock lock(_config_mutex);

    _up_gesture.reset();
    _down_gesture.reset();
    _config = profile.hiresscroll;
    _makeConfig();
}

uint8_t HiresScroll::getMode() {
    return _hires_scroll->getMode();
}

void HiresScroll::setMode(uint8_t mode) {
    _hires_scroll->setMode(mode);
}

void HiresScroll::_makeGesture(std::shared_ptr<actions::Gesture>& gesture,
                               std::optional<config::Gesture>& config,
                               const std::string& direction) {
    if (config.has_value()) {
        gesture = actions::Gesture::makeGesture(_device, config.value(),
                                                _node->make_child(direction));

        _fixGesture(gesture);
    } else {
        gesture.reset();
    }
}

void HiresScroll::_fixGesture(const std::shared_ptr<actions::Gesture>& gesture) {
    try {
        auto axis = std::dynamic_pointer_cast<actions::AxisGesture>(gesture);
        if (axis)
            axis->setHiresMultiplier(_hires_scroll->getCapabilities().multiplier);
    } catch (std::bad_cast& e) {}
    if (gesture)
        gesture->press(true);
}

void HiresScroll::_handleScroll(hidpp20::HiresScroll::WheelStatus event) {
    std::shared_lock lock(_config_mutex);
    auto now = std::chrono::system_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - _last_scroll).count() >= 1) {
        if (_up_gesture) {
            _up_gesture->release(false);
            _up_gesture->press(true);
        }
        if (_down_gesture) {
            _down_gesture->release(false);
            _down_gesture->press(true);
        }

        _last_direction = 0;
    }

    if (event.deltaV > 0) {
        if (_last_direction == -1) {
            if (_down_gesture) {
                _down_gesture->release(false);
                _down_gesture->press(true);
            }
        }
        if (_up_gesture)
            _up_gesture->move(event.deltaV);
        _last_direction = 1;
    } else if (event.deltaV < 0) {
        if (_last_direction == 1) {
            if (_up_gesture) {
                _up_gesture->release(false);
                _up_gesture->press(true);
            }
        }
        if (_down_gesture)
            _down_gesture->move((int16_t) -event.deltaV);
        _last_direction = -1;
    }

    _last_scroll = now;
}

HiresScroll::IPC::IPC(HiresScroll* parent) : ipcgull::interface(
        SERVICE_ROOT_NAME ".HiresScroll", {
                {"GetConfig", {this, &IPC::getConfig, {"hires", "invert", "target"}}},
                {"SetHires",  {this, &IPC::setHires,  {"hires"}}},
                {"SetInvert", {this, &IPC::setInvert, {"invert"}}},
                {"SetTarget", {this, &IPC::setTarget, {"target"}}},
                {"SetUp",     {this, &IPC::setUp,     {"type"}}},
                {"SetDown",   {this, &IPC::setDown,   {"type"}}},
        }, {}, {}), _parent(*parent) {
}

std::tuple<bool, bool, bool> HiresScroll::IPC::getConfig() const {
    std::shared_lock lock(_parent._config_mutex);

    auto& config = _parent._config.get();

    if (config.has_value()) {
        if (std::holds_alternative<bool>(config.value())) {
            return {std::get<bool>(config.value()), false, false};
        } else {
            const auto& config_obj = std::get<config::HiresScroll>(config.value());
            return {
                    config_obj.hires.value_or(true),
                    config_obj.invert.value_or(false),
                    config_obj.target.value_or(false)
            };
        }
    } else {
        return {true, false, false};
    }
}

config::HiresScroll& HiresScroll::IPC::_parentConfig() {
    auto& config = _parent._config.get();
    if (!config.has_value()) {
        config = config::HiresScroll();
    } else if (std::holds_alternative<bool>(config.value())) {
        bool hires = std::get<bool>(config.value());
        auto new_config = config::HiresScroll();
        new_config.hires = hires;
        config = new_config;
    }

    return std::get<config::HiresScroll>(config.value());
}

void HiresScroll::IPC::setHires(bool hires) {
    std::unique_lock lock(_parent._config_mutex);
    _parentConfig().hires = hires;

    _parent._mask |= hidpp20::HiresScroll::Mode::HiRes;
    if (hires)
        _parent._mode |= hidpp20::HiresScroll::Mode::HiRes;
    else
        _parent._mode &= ~hidpp20::HiresScroll::Mode::HiRes;

    _parent._configure();
}

void HiresScroll::IPC::setInvert(bool invert) {
    std::unique_lock lock(_parent._config_mutex);
    _parentConfig().invert = invert;

    _parent._mask |= hidpp20::HiresScroll::Mode::Inverted;
    if (invert)
        _parent._mode |= hidpp20::HiresScroll::Mode::Inverted;
    else
        _parent._mode &= ~hidpp20::HiresScroll::Mode::Inverted;

    _parent._configure();
}

void HiresScroll::IPC::setTarget(bool target) {
    std::unique_lock lock(_parent._config_mutex);
    _parentConfig().target = target;

    _parent._mask |= hidpp20::HiresScroll::Mode::Target;
    if (target)
        _parent._mode |= hidpp20::HiresScroll::Mode::Target;
    else
        _parent._mode &= ~hidpp20::HiresScroll::Mode::Target;

    _parent._configure();
}

void HiresScroll::IPC::setUp(const std::string& type) {
    std::unique_lock lock(_parent._config_mutex);

    auto& config = _parentConfig();

    if (!config.up.has_value()) {
        config.up = config::NoGesture();
    }
    _parent._up_gesture = actions::Gesture::makeGesture(
            _parent._device, type, config.up.value(), _parent._up_node);
    if (!_parent._up_gesture->wheelCompatibility()) {
        _parent._up_node.reset();
        config.up.reset();

        throw std::invalid_argument("incompatible gesture");
    } else {
        _parent._fixGesture(_parent._up_gesture);
    }
}

void HiresScroll::IPC::setDown(const std::string& type) {
    std::unique_lock lock(_parent._config_mutex);

    auto& config = _parentConfig();

    if (!config.down.has_value()) {
        config.down = config::NoGesture();
    }
    _parent._down_gesture = actions::Gesture::makeGesture(
            _parent._device, type, config.down.value(), _parent._down_node);
    if (!_parent._down_gesture->wheelCompatibility()) {
        _parent._down_node.reset();
        config.down.reset();

        throw std::invalid_argument("incompatible gesture");
    } else {
        _parent._fixGesture(_parent._down_gesture);
    }
}
