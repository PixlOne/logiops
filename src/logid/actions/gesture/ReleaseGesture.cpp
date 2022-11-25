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
#include "ReleaseGesture.h"

using namespace logid::actions;

ReleaseGesture::ReleaseGesture(Device *device, libconfig::Setting &root) :
    Gesture (device), _config (device, root)
{
}

void ReleaseGesture::press(bool init_threshold)
{
    _axis = init_threshold ? _config.threshold() : 0;
    _abs_secondary_axis = 0;
}

bool ReleaseGesture::release()
{
    if(_axis >= _config.threshold() && _axis > abs(_abs_secondary_axis)) {
        _config.action()->press();
        _config.action()->release();
        return true;
    }
    return false;
}

void ReleaseGesture::move(int16_t axis, int16_t secondary_axis)
{

    _axis += axis;
    _abs_secondary_axis += abs(secondary_axis);
}

bool ReleaseGesture::wheelCompatibility() const
{
    return false;
}