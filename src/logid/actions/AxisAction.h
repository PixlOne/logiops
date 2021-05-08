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
#ifndef LOGID_ACTION_AXIS_H
#define LOGID_ACTION_AXIS_H

#include <mutex>
#include <vector>
#include <libconfig.h++>
#include "Action.h"

namespace logid {
namespace actions {
    class AxisAction : public Action
    {
    public:
        AxisAction(Device* dev, libconfig::Setting& config);

        virtual void press();
        virtual void release();

        virtual uint8_t reprogFlags() const;

        class Config : public Action::Config
        {
        public:
            explicit Config(Device* device, libconfig::Setting& root);
            uint axis();
            int  move();
            uint rate();
            int hiResMoveMultiplier();
            std::mutex& repeatMutex();
        protected:
            bool registerLowResAxis(const uint axis_code);
            uint _axis;
            int  _move = 1;
            uint _rate = 100;
            int  _hiResMoveMultiplier = 120;
            std::mutex _repeatMutex;
        };
    protected:
        Config _config;
    };
}}

#endif //LOGID_ACTION_KEYPRESS_H
