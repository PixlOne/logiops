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
#ifndef LOGID_FEATURE_HIRESSCROLL_H
#define LOGID_FEATURE_HIRESSCROLL_H

#include "../backend/hidpp20/features/HiresScroll.h"
#include "DeviceFeature.h"
#include "../actions/gesture/Gesture.h"

namespace logid {
namespace features
{
    class HiresScroll : public DeviceFeature
    {
    public:
        explicit HiresScroll(Device* dev);
        ~HiresScroll();
        virtual void configure();
        virtual void listen();

        uint8_t getMode();
        void setMode(uint8_t mode);
    private:
        void _makeAction(std::shared_ptr<actions::Gesture>& gesture,
                         std::optional<config::Gesture>& config);

        void _handleScroll(backend::hidpp20::HiresScroll::WheelStatus event);
        std::shared_ptr<backend::hidpp20::HiresScroll> _hires_scroll;
        std::chrono::time_point<std::chrono::system_clock> _last_scroll;
        int16_t _last_direction = 0;

        std::optional<std::variant<bool, config::HiresScroll>>& _config;

        uint8_t _mode;
        uint8_t _mask;

        std::shared_ptr<actions::Gesture> _up_action;
        std::shared_ptr<actions::Gesture> _down_action;
    };
}}

#endif //LOGID_FEATURE_HIRESSCROLL_H
