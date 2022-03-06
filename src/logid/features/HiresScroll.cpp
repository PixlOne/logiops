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
#include "HiresScroll.h"
#include "../Device.h"
#include "../InputDevice.h"
#include "../actions/gesture/AxisGesture.h"

using namespace logid::features;
using namespace logid::backend;

#define MOVE_EVENTHANDLER_NAME "HIRES_SCROLL"

HiresScroll::HiresScroll(Device *dev) : DeviceFeature(dev),
    _config (dev->activeProfile().hiresscroll), _mode (0), _mask (0),
    _node (dev->ipcNode()->make_child("hires"))
{
    if(_config.has_value()) {
        if(std::holds_alternative<bool>(_config.value())) {
            config::HiresScroll conf {};
            conf.hires = std::get<bool>(_config.value());
            conf.invert = false;
            conf.target = false;
            _mask |= hidpp20::HiresScroll::Mode::HiRes;
            _config.value() = conf;
        }
        auto& conf = std::get<config::HiresScroll>(_config.value());
        if(conf.hires.has_value()) {
            _mask |= hidpp20::HiresScroll::Mode::HiRes;
            if(conf.hires.value())
                _mode |= hidpp20::HiresScroll::Mode::HiRes;
        }
        if(conf.invert.has_value()) {
            _mask |= hidpp20::HiresScroll::Mode::Inverted;
            if(conf.invert.value())
                _mode |= hidpp20::HiresScroll::Mode::Inverted;
        }
        if(conf.target.has_value()) {
            _mask |= hidpp20::HiresScroll::Mode::Target;
            if(conf.target.value())
                _mode |= hidpp20::HiresScroll::Mode::Target;
        }

        _makeAction(_up_action, conf.up, "up");
        _makeAction(_down_action, conf.down, "down");
    }

    try {
        _hires_scroll = std::make_shared<hidpp20::HiresScroll>(&dev->hidpp20());
    } catch(hidpp20::UnsupportedFeature& e) {
        throw UnsupportedFeature();
    }

    _last_scroll = std::chrono::system_clock::now();
}

HiresScroll::~HiresScroll()
{
    if(_ev_handler.has_value())
        _device->hidpp20().removeEventHandler(_ev_handler.value());
}

void HiresScroll::configure()
{
    auto mode = _hires_scroll->getMode();
    mode &= ~_mask;
    mode |= (_mode & _mask);
    _hires_scroll->setMode(mode);
}

void HiresScroll::listen()
{
    if(!_ev_handler.has_value()) {
        _ev_handler = _device->hidpp20().addEventHandler({
            [index=_hires_scroll->featureIndex()](
                    const hidpp::Report& report)->bool {
                return (report.feature() == index) && (report.function() ==
                                                       hidpp20::HiresScroll::WheelMovement);
            },
            [this](const hidpp::Report& report) {
                _handleScroll(_hires_scroll->wheelMovementEvent(report));
            }
        });
    }
}

uint8_t HiresScroll::getMode()
{
    return _hires_scroll->getMode();
}

void HiresScroll::setMode(uint8_t mode)
{
    _hires_scroll->setMode(mode);
}

void HiresScroll::_makeAction(std::shared_ptr<actions::Gesture> &gesture,
                              std::optional<config::Gesture> &config,
                              const std::string& direction)
{
    if(config.has_value()) {
        gesture = actions::Gesture::makeGesture(_device, config.value(),
                                                _node->make_child(direction));
        try {
            auto axis = std::dynamic_pointer_cast<actions::AxisGesture>(
                    gesture);
            if(axis)
                axis->setHiresMultiplier(
                        _hires_scroll->getCapabilities().multiplier);
        } catch(std::bad_cast& e) { }
        if(gesture)
            gesture->press(true);
    } else {
        gesture.reset();
    }
}

void HiresScroll::_handleScroll(hidpp20::HiresScroll::WheelStatus event)
{
    auto now = std::chrono::system_clock::now();
    if(std::chrono::duration_cast<std::chrono::seconds>(
            now - _last_scroll).count() >= 1) {
        if(_up_action) {
            _up_action->release();
            _up_action->press(true);
        }
        if(_down_action) {
            _down_action->release();
            _down_action->press(true);
        }

        _last_direction = 0;
    }

    if(event.deltaV > 0) {
        if(_last_direction == -1) {
            if(_down_action){
                _down_action->release();
                _down_action->press(true);
            }
        }
        if(_up_action)
            _up_action->move(event.deltaV);
        _last_direction = 1;
    } else if(event.deltaV < 0) {
        if(_last_direction == 1) {
            if(_up_action){
                _up_action->release();
                _up_action->press(true);
            }
        }
        if(_down_action)
            _down_action->move(-event.deltaV);
        _last_direction = -1;
    }

    _last_scroll = now;
}
