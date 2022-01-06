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

#ifndef LOGID_BACKEND_HIDPP20_FEATURE_H
#define LOGID_BACKEND_HIDPP20_FEATURE_H

#include <cstdint>
#include "Device.h"

namespace logid {
namespace backend {
namespace hidpp20 {
    class UnsupportedFeature : public std::exception
    {
    public:
        explicit UnsupportedFeature(uint16_t ID) : _f_id (ID) {}
        const char* what() const noexcept override;
        uint16_t code() const noexcept;
    private:
        uint16_t _f_id;
    };

    class Feature
    {
    public:
        static const uint16_t ID;
        virtual uint16_t getID() = 0;
        uint8_t featureIndex();
        virtual ~Feature() = default;
    protected:
        explicit Feature(Device* dev, uint16_t _id);
        std::vector<uint8_t> callFunction(uint8_t function_id,
            std::vector<uint8_t>& params);
        void callFunctionNoResponse(uint8_t function_id,
            std::vector<uint8_t>& params);
    private:
        Device* _device;
        uint8_t _index;
    };
}}}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_H