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
#ifndef LOGID_ACTION_AXISGESTURE_H
#define LOGID_ACTION_AXISGESTURE_H

#include "Gesture.h"

namespace logid {
    namespace actions
    {
        class AxisGesture : public Gesture
        {
        public:
            AxisGesture(Device* device, libconfig::Setting& root);

            virtual void press();
            virtual void release(bool primary=false);
            virtual void move(int16_t axis);

            virtual bool metThreshold() const;

            class Config : public Gesture::Config
            {
            public:
                Config(Device* device, libconfig::Setting& setting);
                unsigned int axis() const;
                double multiplier() const;
            private:
                unsigned int _axis;
                double _multiplier = 1;
            };

        protected:
            int16_t _axis;
            double _axis_remainder;
            Config _config;
        };
    }}

#endif //LOGID_ACTION_AXISGESTURE_H
