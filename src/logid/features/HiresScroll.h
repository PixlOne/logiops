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
#ifndef LOGID_FEATURE_HIRESSCROLL_H
#define LOGID_FEATURE_HIRESSCROLL_H

#include <features/DeviceFeature.h>
#include <actions/gesture/Gesture.h>
#include <backend/hidpp20/features/HiresScroll.h>
#include <backend/hidpp/Device.h>
#include <memory>
#include <optional>
#include <variant>
#include <chrono>

namespace logid::features {
    class HiresScroll : public DeviceFeature {
    public:
        void configure() final;

        void listen() final;

        void setProfile(config::Profile& profile) final;

        [[nodiscard]] uint8_t getMode();

        void setMode(uint8_t mode);

    protected:
        explicit HiresScroll(Device* dev);

    private:
        void _makeConfig();

        EventHandlerLock<backend::hidpp::Device> _ev_handler;

        void _makeGesture(std::shared_ptr<actions::Gesture>& gesture,
                          std::optional<config::Gesture>& config,
                          const std::string& direction);

        void _configure();

        void _fixGesture(const std::shared_ptr<actions::Gesture>& gesture);

        void _handleScroll(backend::hidpp20::HiresScroll::WheelStatus event);

        class IPC : public ipcgull::interface {
        public:
            explicit IPC(HiresScroll* parent);

            [[nodiscard]] std::tuple<bool, bool, bool> getConfig() const;

            void setHires(bool hires);

            void setInvert(bool invert);

            void setTarget(bool target);

            void setUp(const std::string& type);

            void setDown(const std::string& type);

        private:
            config::HiresScroll& _parentConfig();

            HiresScroll& _parent;
        };

        std::shared_ptr<backend::hidpp20::HiresScroll> _hires_scroll;
        std::chrono::time_point<std::chrono::system_clock> _last_scroll;
        int16_t _last_direction = 0;

        mutable std::shared_mutex _config_mutex;
        std::reference_wrapper<std::optional<std::variant<bool, config::HiresScroll>>> _config;

        uint8_t _mode;
        uint8_t _mask;

        std::shared_ptr<actions::Gesture> _up_gesture;
        std::shared_ptr<actions::Gesture> _down_gesture;

        std::shared_ptr<ipcgull::node> _node;
        std::shared_ptr<ipcgull::node> _up_node;
        std::shared_ptr<ipcgull::node> _down_node;

        std::shared_ptr<IPC> _ipc_interface;
    };
}

#endif //LOGID_FEATURE_HIRESSCROLL_H
