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
#ifndef LOGID_BACKEND_HIDPP20_FEATURE_HIRESSCROLL_H
#define LOGID_BACKEND_HIDPP20_FEATURE_HIRESSCROLL_H

#include "../Feature.h"
#include "../feature_defs.h"

namespace logid {
namespace backend {
namespace hidpp20
{
    class HiresScroll : public Feature
    {
    public:
        ///TODO: Hires scroll V1?
        static const uint16_t ID = FeatureID::HIRES_SCROLLING_V2;
        virtual uint16_t getID() { return ID; }

        enum Function : uint8_t
        {
            GetCapabilities = 0,
            GetMode = 1,
            SetMode = 2,
            GetRatchetState = 3
        };

        enum Event : uint8_t
        {
            WheelMovement = 0,
            RatchetSwitch = 1,
        };

        enum Capability : uint8_t
        {
            Invertable = 1<<3,
            HasRatchet = 1<<2
        };

        enum Mode : uint8_t
        {
            Inverted = 1<<2,
            HiRes = 1<<1,
            Target = 1
        };

        enum RatchetState : uint8_t
        {
            FreeWheel = 0,
            Ratchet = 1
        };

        struct Capabilities
        {
            uint8_t multiplier;
            uint8_t flags;
        };

        struct WheelStatus
        {
            bool hiRes;
            uint8_t periods;
            uint16_t deltaV;
        };

        explicit HiresScroll(Device* device);

        Capabilities getCapabilities();
        uint8_t getMode();
        void setMode(uint8_t mode);
        bool getRatchetState();

        ///TODO: Event handlers
        static WheelStatus wheelMovementEvent(const hidpp::Report& report);
        static RatchetState ratchetSwitchEvent(const hidpp::Report& report);
    };
}}}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_HIRESSCROLL_H
