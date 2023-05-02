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
#ifndef LOGID_BACKEND_HIDPP20_FEATURE_THUMBWHEEL_H
#define LOGID_BACKEND_HIDPP20_FEATURE_THUMBWHEEL_H

#include <backend/hidpp20/Feature.h>
#include <backend/hidpp20/feature_defs.h>
#include <backend/hidpp/Report.h>

namespace logid::backend::hidpp20 {
    class ThumbWheel : public Feature {
    public:
        static const uint16_t ID = FeatureID::THUMB_WHEEL;

        uint16_t getID() final { return ID; }

        enum Function {
            GetInfo = 0,
            GetStatus = 1,
            SetReporting = 2
        };

        enum Event {
            Event = 0 /* Catch-all event */
        };

        explicit ThumbWheel(Device* dev);

        enum Capabilities : uint8_t {
            Timestamp = 1,
            Touch = 1 << 1,
            Proxy = 1 << 2,
            SingleTap = 1 << 3
        };

        struct ThumbwheelInfo {
            uint16_t nativeRes;
            uint16_t divertedRes;
            int8_t defaultDirection;
            uint8_t capabilities;
            uint16_t timeElapsed;
        };

        struct ThumbwheelStatus {
            bool diverted;
            bool inverted;
            bool touch;
            bool proxy;
        };

        enum RotationStatus : uint8_t {
            Inactive = 0,
            Start = 1,
            Active = 2,
            Stop = 3
        };

        struct ThumbwheelEvent {
            int16_t rotation;
            uint16_t timestamp;
            RotationStatus rotationStatus;
            uint8_t flags;
        };

        [[nodiscard]] ThumbwheelInfo getInfo();

        [[nodiscard]] ThumbwheelStatus getStatus();

        ThumbwheelStatus setStatus(bool divert, bool invert);

        [[nodiscard]] static ThumbwheelEvent thumbwheelEvent(const hidpp::Report& report);
    };
}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_THUMBWHEEL_H
