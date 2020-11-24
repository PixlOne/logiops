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
#ifndef LOGID_ACTION_GESTUREACTION_H
#define LOGID_ACTION_GESTUREACTION_H

#include <map>
#include <libconfig.h++>
#include "Action.h"
#include "gesture/Gesture.h"

namespace logid {
namespace actions {
    class GestureAction : public Action
    {
    public:
        enum Direction
        {
            None,
            Up,
            Down,
            Left,
            Right,
            ScrollUp,
            ScrollDown,
        };
        static Direction toDirection(std::string direction);
        static Direction toDirection(int16_t x, int16_t y, int16_t s);

        GestureAction(Device* dev, libconfig::Setting& config);

        virtual void press();
        virtual void release();
        virtual void move(int16_t x, int16_t y) {
          move3D(x, y, 0);
        };
        virtual void scroll(int16_t s) {
          move3D(0, 0, s);
        };

        void move3D(int16_t, int16_t, int16_t);

        virtual uint8_t reprogFlags() const;

        class Config : public Action::Config
        {
        public:
            Config(Device* device, libconfig::Setting& root);
            std::map<Direction, std::shared_ptr<Gesture>>& gestures();
            std::shared_ptr<Action> noneAction();
        protected:
            std::map<Direction, std::shared_ptr<Gesture>> _gestures;
            std::shared_ptr<Action> _none_action;
        };

    protected:
        int16_t _x, _y, _s;
        Config _config;
    };
}}

#endif //LOGID_ACTION_GESTUREACTION_H
