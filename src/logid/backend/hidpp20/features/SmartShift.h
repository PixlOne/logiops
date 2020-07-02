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
#ifndef LOGID_BACKEND_HIDPP20_FEATURE_SMARTSHIFT_H
#define LOGID_BACKEND_HIDPP20_FEATURE_SMARTSHIFT_H

#include "../feature_defs.h"
#include "../Feature.h"

namespace logid {
namespace backend {
namespace hidpp20
{
    class SmartShift : public Feature
    {
    public:
        static const uint16_t ID = FeatureID::SMART_SHIFT;
        virtual uint16_t getID() { return ID; }

        enum Function {
            GetStatus = 0,
            SetStatus = 1
        };

        SmartShift(Device* dev);

        struct SmartshiftStatus
        {
            bool active;
            uint8_t autoDisengage;
            uint8_t defaultAutoDisengage;
            bool setActive, setAutoDisengage, setDefaultAutoDisengage;
        };

        SmartshiftStatus getStatus();
        void setStatus(SmartshiftStatus status);
    };
}}}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_SMARTSHIFT_H
