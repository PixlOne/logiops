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
#ifndef LOGID_BACKEND_HIDPP20_FEATURE_ADJUSTABLEDPI_H
#define LOGID_BACKEND_HIDPP20_FEATURE_ADJUSTABLEDPI_H

#include <backend/hidpp20/Feature.h>
#include <backend/hidpp20/feature_defs.h>

namespace logid::backend::hidpp20 {
    class AdjustableDPI : public Feature {
    public:
        static const uint16_t ID = FeatureID::ADJUSTABLE_DPI;

        [[nodiscard]] uint16_t getID() final { return ID; }

        enum Function {
            GetSensorCount = 0,
            GetSensorDPIList = 1,
            GetSensorDPI = 2,
            SetSensorDPI = 3
        };

        explicit AdjustableDPI(Device* dev);

        uint8_t getSensorCount();

        struct SensorDPIList {
            std::vector<uint16_t> dpis;
            bool isRange;
            uint16_t dpiStep;
        };

        SensorDPIList getSensorDPIList(uint8_t sensor);

        uint16_t getDefaultSensorDPI(uint8_t sensor);

        uint16_t getSensorDPI(uint8_t sensor);

        void setSensorDPI(uint8_t sensor, uint16_t dpi);
    };
}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_ADJUSTABLEDPI_H
