/*
 * Copyright 2025 Kristóf Marussy
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
#ifndef LOGID_BACKEND_HIDPP20_FEATURE_HAPTICFEEDBACK_H
#define LOGID_BACKEND_HIDPP20_FEATURE_HAPTICFEEDBACK_H

#include <backend/hidpp20/Feature.h>
#include <backend/hidpp20/feature_defs.h>

namespace logid::backend::hidpp20 {
    class HapticFeedback : public Feature {
    public:
        static const uint16_t ID = FeatureID::HAPTIC_FEEDBACK;

        [[nodiscard]] uint16_t getID() final { return ID; }

        enum Function {
            SetStrength = 2,
            PlayEffect = 4
        };

        explicit HapticFeedback(Device* dev);

        void setStrength(uint8_t strength, bool enabled = true, bool battery_saving = false);

        void playEffect(uint8_t effect);
    };
}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_HAPTICFEEDBACK_H
