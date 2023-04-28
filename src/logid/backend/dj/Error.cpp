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

#include "Error.h"

using namespace logid::backend::dj;

Error::Error(uint8_t code) : _code(code) {
}

const char* Error::what() const noexcept {
    switch (_code) {
        case Unknown:
            return "Unknown";
        case KeepAliveTimeout:
            return "Keep-alive timeout";
        default:
            return "Reserved";
    }
}

uint8_t Error::code() const noexcept {
    return _code;
}