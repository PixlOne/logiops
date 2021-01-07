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

void AxisGesture::press(bool init_threshold)
{
    _axis = init_threshold ? _config.threshold() : 0;
    _axis_remainder = 0;
    _hires_remainder = 0;
}

void AxisGesture::release(bool primary)
{
    // Do nothing
    (void)primary; // Suppress unused warning
}

void AxisGesture::move(int16_t axis)
{
    int16_t new_axis = _axis+axis;
    int low_res_axis = InputDevice::getLowResAxis(_config.axis());
    int hires_remainder = _hires_remainder;

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

        if(low_res_axis != -1) {
            int lowres_movement = 0, hires_movement = move_floor;
            virtual_input->moveAxis(_config.axis(), hires_movement);
            hires_remainder += hires_movement;
            if(abs(hires_remainder) >= 60) {
                lowres_movement = hires_remainder/120;
                if(lowres_movement == 0)
                    lowres_movement = hires_remainder > 0 ? 1 : -1;
                hires_remainder -= lowres_movement*120;
                virtual_input->moveAxis(low_res_axis, lowres_movement);
            }

            _hires_remainder = hires_remainder;
        } else {
            virtual_input->moveAxis(_config.axis(), move_floor);
        }
    }
    _axis = new_axis;
}

bool AxisGesture::metThreshold() const
{
    return _axis >= _config.threshold();
}

void AxisGesture::setHiresMultiplier(double multiplier)
{
    _config.setHiresMultiplier(multiplier);
}

AxisGesture::Config::Config(Device *device, libconfig::Setting &setting) :
    Gesture::Config(device, setting, false)
{
    try {
        auto& axis = setting.lookup("axis");
        if(axis.isNumber()) {
            _axis = axis;
            virtual_input->registerAxis(_axis);
        } else if(axis.getType() == libconfig::Setting::TypeString) {
            try {
                _axis = virtual_input->toAxisCode(axis);
                virtual_input->registerAxis(_axis);
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

    int low_res_axis = InputDevice::getLowResAxis(_axis);
    if(low_res_axis != -1) {
        _multiplier *= 120;
        virtual_input->registerAxis(low_res_axis);
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

bool AxisGesture::wheelCompatibility() const
{
    return true;
}

void AxisGesture::Config::setHiresMultiplier(double multiplier)
{
    if(_hires_multiplier == multiplier || multiplier == 0)
        return;

    if(InputDevice::getLowResAxis(_axis) != -1) {
        _multiplier *= _hires_multiplier;
        _multiplier /= multiplier;
    }

    _hires_multiplier = multiplier;
}
