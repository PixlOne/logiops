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

#include <backend/hidpp20/Error.h>
#include <cassert>

using namespace logid::backend;
using namespace logid::backend::hidpp20;

Error::Error(uint8_t code, hidpp::DeviceIndex index) : _code(code), _index (index) {
    assert(_code != NoError);
}

const char* Error::what() const noexcept {
    switch (_code) {
        case NoError:
            return "No error";
        case Unknown:
            return "Unknown";
        case InvalidArgument:
            return "Invalid argument";
        case OutOfRange:
            return "Out of range";
        case HardwareError:
            return "Hardware error";
        case LogitechInternal:
            return "Logitech internal feature";
        case InvalidFeatureIndex:
            return "Invalid feature index";
        case InvalidFunctionID:
            return "Invalid function ID";
        case Busy:
            return "Busy";
        case Unsupported:
            return "Unsupported";
        case UnknownDevice:
            return "Unknown device";
        default:
            return "Unknown error code";
    }
}

uint8_t Error::code() const noexcept {
    return _code;
}

hidpp::DeviceIndex Error::deviceIndex() const noexcept {
    return _index;
}