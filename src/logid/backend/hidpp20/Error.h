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

#ifndef LOGID_BACKEND_HIDPP20_ERROR_H
#define LOGID_BACKEND_HIDPP20_ERROR_H

#include <backend/hidpp/defs.h>
#include <stdexcept>
#include <cstdint>

namespace logid::backend::hidpp20 {
    static constexpr uint8_t ErrorID = 0xFF;

    class Error : public std::exception {
    public:
        enum ErrorCode : uint8_t {
            NoError = 0,
            Unknown = 1,
            InvalidArgument = 2,
            OutOfRange = 3,
            HardwareError = 4,
            LogitechInternal = 5,
            InvalidFeatureIndex = 6,
            InvalidFunctionID = 7,
            Busy = 8,
            Unsupported = 9,
            UnknownDevice = 10
        };

        Error(uint8_t code, hidpp::DeviceIndex index);

        [[nodiscard]] const char* what() const noexcept override;

        [[nodiscard]] uint8_t code() const noexcept;

        [[nodiscard]] hidpp::DeviceIndex deviceIndex() const noexcept;

    private:
        uint8_t _code;
        hidpp::DeviceIndex _index;
    };
}

#endif //LOGID_BACKEND_HIDPP20_ERROR_H