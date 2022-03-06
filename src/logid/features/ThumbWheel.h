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
#ifndef LOGID_FEATURE_THUMBWHEEL_H
#define LOGID_FEATURE_THUMBWHEEL_H

#include "../backend/hidpp20/features/ThumbWheel.h"
#include "DeviceFeature.h"
#include "../actions/gesture/Gesture.h"
#include "../config/schema.h"

namespace logid {
namespace features
{
    class ThumbWheel : public DeviceFeature
    {
    public:
        explicit ThumbWheel(Device* dev);
        ~ThumbWheel();
        virtual void configure();
        virtual void listen();

    private:
        void _handleEvent(backend::hidpp20::ThumbWheel::ThumbwheelEvent event);

        std::shared_ptr<backend::hidpp20::ThumbWheel> _thumb_wheel;
        backend::hidpp20::ThumbWheel::ThumbwheelInfo _wheel_info;

        std::shared_ptr<ipcgull::node> _node;
        std::shared_ptr<ipcgull::node> _gesture_node;

        std::shared_ptr<actions::Gesture> _left_action;
        std::shared_ptr<actions::Gesture> _right_action;
        std::shared_ptr<actions::Action> _proxy_action;
        std::shared_ptr<ipcgull::node> _proxy_node;
        std::shared_ptr<actions::Action> _tap_action;
        std::shared_ptr<ipcgull::node> _tap_node;
        std::shared_ptr<actions::Action> _touch_action;
        std::shared_ptr<ipcgull::node> _touch_node;

        int8_t _last_direction = 0;
        bool _last_proxy = false;
        bool _last_touch = false;
        std::optional<config::ThumbWheel>& _config;

        std::optional<backend::hidpp::Device::EvHandlerId> _ev_handler;
    };
}}

#endif //LOGID_FEATURE_THUMBWHEEL_H
