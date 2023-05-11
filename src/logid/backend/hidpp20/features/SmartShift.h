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
#ifndef LOGID_BACKEND_HIDPP20_FEATURE_SMARTSHIFT_H
#define LOGID_BACKEND_HIDPP20_FEATURE_SMARTSHIFT_H

#include <backend/hidpp20/feature_defs.h>
#include <backend/hidpp20/Feature.h>
#include <memory>

namespace logid::backend::hidpp20 {
    class SmartShift : public Feature {
    public:
        static const uint16_t ID = FeatureID::SMART_SHIFT;

        uint16_t getID() override { return ID; }

        enum Function {
            GetStatus = 0,
            SetStatus = 1
        };

        explicit SmartShift(Device* dev);

        struct Defaults {
            uint8_t autoDisengage;
            uint8_t torque;
            uint8_t maxForce;
        };

        struct Status {
            bool active;
            uint8_t autoDisengage;
            uint8_t torque;
            bool setActive, setAutoDisengage, setTorque;
        };

        [[nodiscard]] virtual bool supportsTorque() { return false; }

        [[nodiscard]] virtual Defaults getDefaults();

        [[nodiscard]] virtual Status getStatus();

        virtual void setStatus(Status status);

        [[nodiscard]] static std::shared_ptr<SmartShift> autoVersion(Device* dev);

    protected:
        SmartShift(Device* dev, uint16_t feature_id);
    };

    class SmartShiftV2 : public SmartShift
    {
    public:
        static const uint16_t ID = FeatureID::SMART_SHIFT_V2;
        uint16_t getID() final { return ID; }

        enum Function {
            GetCapabilities = 0,
            GetStatus = 1,
            SetStatus = 2
        };

        explicit SmartShiftV2(Device* dev);

        [[nodiscard]] bool supportsTorque() final;

        [[nodiscard]] Defaults getDefaults() final;

        [[nodiscard]] Status getStatus() final;

        void setStatus(Status status) final;
    };
}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_SMARTSHIFT_H
