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
#include "IntervalGesture.h"
#include "../../util/log.h"

using namespace logid::actions;

IntervalGesture::IntervalGesture(Device *device, libconfig::Setting &root) :
    Gesture (device), _config (device, root)
{
}

void IntervalGesture::press(bool init_threshold)
{
    _axis = init_threshold ? _config.threshold() : 0;
    _interval_pass_count = 0;
}

void IntervalGesture::release(bool primary)
{
    // Do nothing
    (void)primary; // Suppress unused warning
    _config.action()->release();
}

void IntervalGesture::move(int16_t axis)
{
    _axis += axis;
    if(_axis < _config.threshold())
        return;

    int16_t new_interval_count = (_axis - _config.threshold())/
            _config.interval();
    if(new_interval_count > _interval_pass_count) {
        _config.action()->press();
        _config.action()->secondaryRelease();
    }
    _interval_pass_count = new_interval_count;
}

bool IntervalGesture::wheelCompatibility() const
{
    return true;
}

bool IntervalGesture::metThreshold() const
{
    return _axis >= _config.threshold();
}

IntervalGesture::Config::Config(Device *device, libconfig::Setting &setting) :
    Gesture::Config(device, setting)
{
    try {
        auto& interval = setting["interval"];
        if(interval.getType() != libconfig::Setting::TypeInt) {
            logPrintf(WARN, "Line %d: interval must be an integer, skipping.",
                    interval.getSourceLine());
            throw InvalidGesture();
        }
        _interval = (int)interval;
    } catch(libconfig::SettingNotFoundException& e) {
        try {
            // pixels is an alias for interval
            auto& interval = setting["pixels"];
            if(interval.getType() != libconfig::Setting::TypeInt) {
                logPrintf(WARN, "Line %d: pixels must be an integer, skipping.",
                          interval.getSourceLine());
                throw InvalidGesture();
            }
            _interval = (int)interval;
        } catch(libconfig::SettingNotFoundException& e) {
            logPrintf(WARN, "Line %d: interval is a required field, skipping.",
                      setting.getSourceLine());
        }
    }
}

int16_t IntervalGesture::Config::interval() const
{
    return _interval;
}
