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
#ifndef LOGID_FEATURE_THUMBWHEEL_H
#define LOGID_FEATURE_THUMBWHEEL_H

#include <features/DeviceFeature.h>
#include <actions/gesture/Gesture.h>
#include <backend/hidpp20/features/ThumbWheel.h>
#include <backend/hidpp/Device.h>
#include <chrono>
#include <atomic>

namespace logid::features {
    class ThumbWheel : public DeviceFeature {
    public:
        explicit ThumbWheel(Device* dev);

        void configure() final;

        void listen() final;

        void setProfile(config::Profile& profile) final;

    private:
        void _makeConfig();

        void _handleEvent(backend::hidpp20::ThumbWheel::ThumbwheelEvent event);

        void _fixGesture(const std::shared_ptr<actions::Gesture>& gesture) const;

        class IPC : public ipcgull::interface {
        public:
            explicit IPC(ThumbWheel* parent);

            [[nodiscard]] std::tuple<bool, bool> getConfig() const;

            void setDivert(bool divert);

            void setInvert(bool invert);

            void setLeft(const std::string& type);

            void setRight(const std::string& type);

            void setProxy(const std::string& type);

            void setTap(const std::string& type);

            void setTouch(const std::string& type);

        private:
            config::ThumbWheel& _parentConfig();

            ThumbWheel& _parent;
        };

        std::shared_ptr<backend::hidpp20::ThumbWheel> _thumb_wheel;
        backend::hidpp20::ThumbWheel::ThumbwheelInfo _wheel_info;

        std::shared_ptr<ipcgull::node> _node;

        std::shared_ptr<actions::Gesture> _left_gesture;
        std::shared_ptr<ipcgull::node> _left_node;
        std::shared_ptr<actions::Gesture> _right_gesture;
        std::shared_ptr<ipcgull::node> _right_node;
        std::shared_ptr<actions::Action> _proxy_action;
        std::shared_ptr<ipcgull::node> _proxy_node;
        std::shared_ptr<actions::Action> _tap_action;
        std::shared_ptr<ipcgull::node> _tap_node;
        std::shared_ptr<actions::Action> _touch_action;
        std::shared_ptr<ipcgull::node> _touch_node;

        bool _last_proxy = false;
        bool _last_touch = false;
        
        // Smart touch detection
        std::chrono::system_clock::time_point _touch_time;
        std::atomic<bool> _touch_pending{false};
        std::atomic<bool> _touch_active{false};
        static constexpr int TOUCH_DELAY_MS = 150;

        mutable std::shared_mutex _config_mutex;
        std::reference_wrapper<std::optional<config::ThumbWheel>> _config;

        EventHandlerLock<backend::hidpp::Device> _ev_handler;

        std::shared_ptr<IPC> _ipc_interface;
    };
}

#endif //LOGID_FEATURE_THUMBWHEEL_H
