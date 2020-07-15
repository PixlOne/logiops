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
#ifndef LOGID_BACKEND_HIDPP20_FEATURE_RESET_H
#define LOGID_BACKEND_HIDPP20_FEATURE_RESET_H

#include "../Feature.h"
#include "../feature_defs.h"

namespace logid {
namespace backend {
namespace hidpp20
{
    class Reset : public Feature
    {
    public:
        static const uint16_t ID = FeatureID::RESET;
        virtual uint16_t getID() { return ID; }

        enum Function : uint8_t
        {
             GetProfile = 0,
             ResetToProfile = 1
        };

        explicit Reset(Device* device);

        uint16_t getProfile();
        void reset(uint16_t profile = 0);
    };
}}}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_RESET_H
