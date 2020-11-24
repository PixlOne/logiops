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
#include "RemapButton.h"
#include "../Device.h"
#include "../InputDevice.h"
#include "../actions/gesture/Gesture.h"
#include "../actions/gesture/AxisGesture.h"

using namespace logid::features;
using namespace logid::backend;

#define MOVE_EVENTHANDLER_NAME "HIRES_SCROLL"

HiresScroll::HiresScroll(Device *dev) : DeviceFeature(dev), _config(dev)
{
    try {
        _hires_scroll = std::make_shared<hidpp20::HiresScroll>(&dev->hidpp20());
    } catch(hidpp20::UnsupportedFeature& e) {
        throw UnsupportedFeature();
    }

    if(_config.upAction()) {
        try {
            auto up_axis = std::dynamic_pointer_cast<actions::AxisGesture>(
                    _config.upAction());
            if(up_axis)
                up_axis->setHiresMultiplier(
                        _hires_scroll->getCapabilities().multiplier);
        } catch(std::bad_cast& e) { }

        _config.upAction()->press(true);
    }

    if(_config.downAction()) {
        try {
            auto down_axis = std::dynamic_pointer_cast<actions::AxisGesture>(
                    _config.downAction());
            if(down_axis)
                down_axis->setHiresMultiplier(
                        _hires_scroll->getCapabilities().multiplier);
        } catch(std::bad_cast& e) { }

        _config.downAction()->press(true);
    }

    _last_scroll = std::chrono::system_clock::now();
}

HiresScroll::~HiresScroll()
{
    _device->hidpp20().removeEventHandler(MOVE_EVENTHANDLER_NAME);
}

void HiresScroll::configure()
{
    auto mode = _hires_scroll->getMode();
    mode &= ~_config.getMask();
    mode |= (_config.getMode() & _config.getMask());
    _hires_scroll->setMode(mode);
}

void HiresScroll::listen()
{
    if(_device->hidpp20().eventHandlers().find(MOVE_EVENTHANDLER_NAME) ==
       _device->hidpp20().eventHandlers().end()) {
        auto handler = std::make_shared<hidpp::EventHandler>();
        handler->condition = [index=_hires_scroll->featureIndex()]
                (hidpp::Report& report)->bool {
            return (report.feature() == index) && (report.function() ==
                hidpp20::HiresScroll::WheelMovement);
        };

        handler->callback = [this](hidpp::Report& report)->void {
            this->_handleScroll(_hires_scroll->wheelMovementEvent(report));
        };

        _device->hidpp20().addEventHandler(MOVE_EVENTHANDLER_NAME, handler);
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

void HiresScroll::_handleScroll(hidpp20::HiresScroll::WheelStatus event)
{
    if (_device->getFeature<features::RemapButton>("remapbutton")->onHiresScroll(event.deltaV)) return;

    auto now = std::chrono::system_clock::now();
    if(std::chrono::duration_cast<std::chrono::seconds>(
            now - _last_scroll).count() >= 1) {
        if(_config.upAction()) {
            _config.upAction()->release();
            _config.upAction()->press(true);
        }
        if(_config.downAction()) {
            _config.downAction()->release();
            _config.downAction()->press(true);
        }

        _last_direction = 0;
    }

    if(event.deltaV > 0) {
        if(_last_direction == -1) {
            if(_config.downAction()){
                _config.downAction()->release();
                _config.downAction()->press(true);
            }
        }
        if(_config.upAction())
            _config.upAction()->move(event.deltaV);
        _last_direction = 1;
    } else if(event.deltaV < 0) {
        if(_last_direction == 1) {
            if(_config.upAction()){
                _config.upAction()->release();
                _config.upAction()->press(true);
            }
        }
        if(_config.downAction())
            _config.downAction()->move(-event.deltaV);
        _last_direction = -1;
    }

    _last_scroll = now;
}

HiresScroll::Config::Config(Device *dev) : DeviceFeature::Config(dev)
{
    try {
        auto& config_root = dev->config().getSetting("hiresscroll");
        if(!config_root.isGroup()) {
            logPrintf(WARN, "Line %d: hiresscroll must be a group",
                      config_root.getSourceLine());
            return;
        }
        _mode = 0;
        _mask = 0;
        try {
            auto& hires = config_root["hires"];
            if(hires.getType() == libconfig::Setting::TypeBoolean) {
                _mask |= hidpp20::HiresScroll::Mode::HiRes;
                if(hires)
                    _mode |= hidpp20::HiresScroll::Mode::HiRes;
            } else {
                logPrintf(WARN, "Line %d: hires must be a boolean",
                    hires.getSourceLine());
            }
        } catch(libconfig::SettingNotFoundException& e) { }

        try {
            auto& invert = config_root["invert"];
            if(invert.getType() == libconfig::Setting::TypeBoolean) {
                _mask |= hidpp20::HiresScroll::Mode::Inverted;
                if(invert)
                    _mode |= hidpp20::HiresScroll::Mode::Inverted;
            } else {
                logPrintf(WARN, "Line %d: invert must be a boolean, ignoring.",
                          invert.getSourceLine());
            }
        } catch(libconfig::SettingNotFoundException& e) { }

        try {
            auto& target = config_root["target"];
            if(target.getType() == libconfig::Setting::TypeBoolean) {
                _mask |= hidpp20::HiresScroll::Mode::Target;
                if(target)
                    _mode |= hidpp20::HiresScroll::Mode::Target;
            } else {
                logPrintf(WARN, "Line %d: target must be a boolean, ignoring.",
                          target.getSourceLine());
            }
        } catch(libconfig::SettingNotFoundException& e) { }

        if(_mode & hidpp20::HiresScroll::Mode::Target) {
            try {
                auto& up = config_root["up"];
                try {
                    auto g = actions::Gesture::makeGesture(dev, up);
                    if(g->wheelCompatibility()) {
                        _up_action = g;
                    } else {
                        logPrintf(WARN, "Line %d: This gesture cannot be used"
                                        " as a scroll action.",
                                        up.getSourceLine());
                    }
                } catch(actions::InvalidGesture& e) {
                    logPrintf(WARN, "Line %d: Invalid scroll action",
                            up.getSourceLine());
                }
            } catch(libconfig::SettingNotFoundException&) {
                logPrintf(WARN, "Line %d: target is true but no up action was"
                                " set, using default", config_root.getSourceLine());
                libconfig::Config c;
                c.getRoot().add("axis", libconfig::Setting::TypeString);
                c.getRoot()["axis"] = "REL_WHEEL";
                c.getRoot().add("axis_multiplier", libconfig::Setting::TypeInt);
                c.getRoot()["axis_multiplier"] = 1;
                _up_action = std::make_shared<actions::AxisGesture>(dev, c.getRoot());
            }

            try {
                auto& down = config_root["down"];
                try {
                    auto g = actions::Gesture::makeGesture(dev, down);
                    if(g->wheelCompatibility()) {
                        _down_action = g;
                    } else {
                        logPrintf(WARN, "Line %d: This gesture cannot be used"
                                        " as a scroll action.",
                                  down.getSourceLine());
                    }
                } catch(actions::InvalidGesture& e) {
                    logPrintf(WARN, "Line %d: Invalid scroll action",
                              down.getSourceLine());
                }
            } catch(libconfig::SettingNotFoundException&) {
                logPrintf(WARN, "Line %d: target is true but no down action was"
                                " set, using default", config_root.getSourceLine());
                libconfig::Config c;
                c.getRoot().add("axis", libconfig::Setting::TypeString);
                c.getRoot()["axis"] = "REL_WHEEL";
                c.getRoot().add("axis_multiplier", libconfig::Setting::TypeInt);
                c.getRoot()["axis_multiplier"] = -1;
                _down_action = std::make_shared<actions::AxisGesture>(dev, c.getRoot());
            }
        }
    } catch(libconfig::SettingNotFoundException& e) {
        // HiresScroll not configured, use default
    }
}

uint8_t HiresScroll::Config::getMode() const
{
    return _mode;
}

uint8_t HiresScroll::Config::getMask() const
{
    return _mask;
}

const std::shared_ptr<logid::actions::Gesture>&
        HiresScroll::Config::upAction() const
{
    return _up_action;
}

const std::shared_ptr<logid::actions::Gesture>&
        HiresScroll::Config::downAction() const
{
    return _down_action;
}
