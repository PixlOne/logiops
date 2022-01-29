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

#include <algorithm>
#include "Gesture.h"
#include "ReleaseGesture.h"
#include "ThresholdGesture.h"
#include "IntervalGesture.h"
#include "AxisGesture.h"
#include "NullGesture.h"

using namespace logid;
using namespace logid::actions;

Gesture::Gesture(Device *device,
                 const std::shared_ptr<ipcgull::node>& parent,
                 const std::string& direction) : _device (device),
                 _node (parent->make_child(direction))
{
}

template <typename T>
struct gesture_type {
    typedef typename T::gesture type;
};

template <typename T>
struct gesture_type<const T> : gesture_type<T> { };

template <typename T>
struct gesture_type<T&> : gesture_type<T> { };

template <typename T>
std::shared_ptr<Gesture> _makeGesture(
        Device* device, T gesture,
        const std::shared_ptr<ipcgull::node>& parent,
        const std::string& direction) {
    return std::make_shared<typename gesture_type<T>::type>(
            device, gesture, parent, std::move(direction));
}

std::shared_ptr<Gesture> Gesture::makeGesture(
        Device *device, config::Gesture& gesture,
        const std::shared_ptr<ipcgull::node>& parent,
        const std::string& direction)
{
    std::shared_ptr<Gesture> ret;
    std::visit([&device, &ret, &parent, &direction](auto&& x) {
        ret = _makeGesture(device, x, parent, direction);
    }, gesture);
    return ret;
}
