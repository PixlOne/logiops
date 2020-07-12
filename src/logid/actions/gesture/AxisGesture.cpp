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
#include <cmath>
#include "AxisGesture.h"
#include "../../InputDevice.h"
#include "../../util/log.h"

using namespace logid::actions;

AxisGesture::AxisGesture(Device *device, libconfig::Setting &root) :
    Gesture (device), _config (device, root)
{
}

void AxisGesture::press()
{
    _axis = 0;
    _axis_remainder = 0;
}

void AxisGesture::release(bool primary)
{
    // Do nothing
    (void)primary; // Suppress unused warning
}

void AxisGesture::move(int16_t axis)
{
    int16_t new_axis = _axis + axis;
    if(new_axis > _config.threshold()) {
        double move = axis;
        if(_axis < _config.threshold())
            move = new_axis - _config.threshold();
        bool negative_multiplier = _config.multiplier() < 0;
        if(negative_multiplier)
            move *= -_config.multiplier();
        else
            move *= _config.multiplier();

        double move_floor = floor(move);
        _axis_remainder = move - move_floor;
        if(_axis_remainder >= 1) {
            double int_remainder = floor(_axis_remainder);
            move_floor += int_remainder;
            _axis_remainder -= int_remainder;
        }

        if(negative_multiplier)
            move_floor = -move_floor;

        virtual_input->moveAxis(_config.axis(), move_floor);
    }
    _axis = new_axis;
}

bool AxisGesture::metThreshold() const
{
    return _axis >= _config.threshold();
}

AxisGesture::Config::Config(Device *device, libconfig::Setting &setting) :
    Gesture::Config(device, setting, false)
{
    try {
        auto& axis = setting.lookup("axis");
        if(axis.isNumber()) {
            _axis = axis;
        } else if(axis.getType() == libconfig::Setting::TypeString) {
            try {
                _axis = virtual_input->toAxisCode(axis);
            } catch(InputDevice::InvalidEventCode& e) {
                logPrintf(WARN, "Line %d: Invalid axis %s, skipping."
                        , axis.getSourceLine(), axis.c_str());
            }
        } else {
            logPrintf(WARN, "Line %d: axis must be string or int, skipping.",
                      axis.getSourceLine(), axis.c_str());
            throw InvalidGesture();
        }
    } catch(libconfig::SettingNotFoundException& e) {
        logPrintf(WARN, "Line %d: axis is a required field, skippimg.",
                setting.getSourceLine());
        throw InvalidGesture();
    }

    try {
        auto& multiplier = setting.lookup("axis_multiplier");
        if(multiplier.isNumber()) {
            if(multiplier.getType() == libconfig::Setting::TypeFloat)
                _multiplier = multiplier;
            else
                _multiplier = (int)multiplier;
        } else {
            logPrintf(WARN, "Line %d: axis_multiplier must be a number, "
                            "setting to default (1).",
                            multiplier.getSourceLine());
        }
    } catch(libconfig::SettingNotFoundException& e) {
        // Ignore
    }
}

unsigned int AxisGesture::Config::axis() const
{
    return _axis;
}

double AxisGesture::Config::multiplier() const
{
    return _multiplier;
}