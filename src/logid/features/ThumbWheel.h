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

namespace logid {
namespace features
{
    class ThumbWheel : public DeviceFeature
    {
    public:
        explicit ThumbWheel(Device* dev);
        virtual void configure();
        virtual void listen();

        class Config : public DeviceFeature::Config
        {
        public:
            explicit Config(Device* dev);
            bool divert() const;
            bool invert() const;

            const std::shared_ptr<actions::Gesture>& leftAction() const;
            const std::shared_ptr<actions::Gesture>& rightAction() const;
            const std::shared_ptr<actions::Action>& proxyAction() const;
            const std::shared_ptr<actions::Action>& tapAction() const;
            const std::shared_ptr<actions::Action>& touchAction() const;
        protected:
            bool _divert = false;
            bool _invert = false;

            static std::shared_ptr<actions::Gesture> _genGesture(Device* dev,
                    libconfig::Setting& setting, const std::string& name);
            static std::shared_ptr<actions::Action> _genAction(Device* dev,
                    libconfig::Setting& setting, const std::string& name);

            std::shared_ptr<actions::Gesture> _left_action;
            std::shared_ptr<actions::Gesture> _right_action;
            std::shared_ptr<actions::Action> _proxy_action;
            std::shared_ptr<actions::Action> _tap_action;
            std::shared_ptr<actions::Action> _touch_action;
        };
    private:
        void _handleEvent(backend::hidpp20::ThumbWheel::ThumbwheelEvent event);

        std::shared_ptr<backend::hidpp20::ThumbWheel> _thumb_wheel;
        backend::hidpp20::ThumbWheel::ThumbwheelInfo _wheel_info;
        int8_t _last_direction = 0;
        bool _last_proxy = false;
        bool _last_touch = false;
        Config _config;
    };
}}

#endif //LOGID_FEATURE_THUMBWHEEL_H
