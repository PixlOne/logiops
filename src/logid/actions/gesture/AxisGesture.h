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
#ifndef LOGID_ACTION_AXISGESTURE_H
#define LOGID_ACTION_AXISGESTURE_H

#include <actions/gesture/Gesture.h>

namespace logid::actions {
    class AxisGesture : public Gesture {
    public:
        static const char* interface_name;

        AxisGesture(Device* device, config::AxisGesture& config,
                    const std::shared_ptr<ipcgull::node>& parent);

        void press(bool init_threshold) final;

        void release(bool primary) final;

        void move(int16_t axis) final;

        [[nodiscard]] bool wheelCompatibility() const final;

        [[nodiscard]] bool metThreshold() const final;

        void setHiresMultiplier(double multiplier);

        [[nodiscard]] std::tuple<std::string, double, int> getConfig() const;

        void setAxis(const std::string& axis);

        void setMultiplier(double multiplier);

        void setThreshold(int threshold);

    protected:
        int32_t _axis{};
        double _axis_remainder{};
        int _hires_remainder{};
        std::optional<uint> _input_axis;
        double _multiplier;
        double _hires_multiplier = 1.0;
        config::AxisGesture& _config;
    };
}

#endif //LOGID_ACTION_AXISGESTURE_H
