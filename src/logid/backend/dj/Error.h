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

#ifndef LOGID_BACKEND_DJ_ERROR_H
#define LOGID_BACKEND_DJ_ERROR_H

#include <cstdint>
#include <stdexcept>

namespace logid {
namespace backend {
namespace dj
{
    class Error : public std::exception
    {
    public:
        enum ErrorCode : uint8_t
        {
            Unknown = 0x00,
            KeepAliveTimeout = 0x01
        };

        explicit Error(uint8_t code);

        const char* what() const noexcept override;
        uint8_t code() const noexcept;

    private:
        uint8_t _code;
    };
}}}

#endif //LOGID_BACKEND_DJ_ERROR_H