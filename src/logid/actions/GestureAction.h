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
#ifndef LOGID_ACTION_GESTUREACTION_H
#define LOGID_ACTION_GESTUREACTION_H

#include <map>
#include <actions/Action.h>
#include <actions/gesture/Gesture.h>

namespace logid::actions {
    class GestureAction : public Action {
    public:
        static const char* interface_name;

        enum Direction {
            None,
            Up,
            Down,
            Left,
            Right
        };

        static Direction toDirection(std::string direction);

        static std::string fromDirection(Direction direction);

        static Direction toDirection(int32_t x, int32_t y);

        GestureAction(Device* dev, config::GestureAction& config,
                      const std::shared_ptr<ipcgull::node>& parent);

        void press() final;

        void release() final;

        void move(int16_t x, int16_t y) final;

        uint8_t reprogFlags() const final;

        void setGesture(const std::string& direction,
                        const std::string& type);

    protected:
        int32_t _x{}, _y{};
        std::shared_ptr<ipcgull::node> _node;
        std::map<Direction, std::shared_ptr<Gesture>> _gestures;
        config::GestureAction& _config;
    };
}

#endif //LOGID_ACTION_GESTUREACTION_H
