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

using namespace logid::features;
using namespace logid::backend;

HiresScroll::HiresScroll(Device *dev) : DeviceFeature(dev),
    _hires_scroll(&dev->hidpp20()), _config(dev)
{
}

void HiresScroll::configure()
{
    auto mode = _hires_scroll.getMode();
    mode &= ~_config.getMask();
    mode |= (_config.getMode() & _config.getMask());
    _hires_scroll.setMode(mode);
}

void HiresScroll::listen()
{
    ///TODO: Map hires scroll events
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
            auto& hires = config_root.lookup("hires");
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
            auto& invert = config_root.lookup("invert");
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
            auto& target = config_root.lookup("target");
            if(target.getType() == libconfig::Setting::TypeBoolean) {
                _mask |= hidpp20::HiresScroll::Mode::Target;
                if(target)
                    _mode |= hidpp20::HiresScroll::Mode::Target;
            } else {
                logPrintf(WARN, "Line %d: target must be a boolean, ignoring.",
                          target.getSourceLine());
            }
        } catch(libconfig::SettingNotFoundException& e) { }
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