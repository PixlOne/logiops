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
#ifndef LOGID_FEATURE_REMAPBUTTON_H
#define LOGID_FEATURE_REMAPBUTTON_H

#include "../backend/hidpp20/features/ReprogControls.h"
#include "DeviceFeature.h"
#include "../actions/Action.h"

namespace logid::features {
    class RemapButton;

    class Button : public ipcgull::object {
    public:
        typedef backend::hidpp20::ReprogControls::ControlInfo Info;
        typedef std::function<void(std::shared_ptr<actions::Action>)>
                ConfigFunction;

        static std::shared_ptr<Button> make(
                Info info, int index, Device* device, ConfigFunction conf_func,
                const std::shared_ptr<ipcgull::node>& root, config::Button& config);

        void press() const;

        void release() const;

        void move(int16_t x, int16_t y) const;

        [[nodiscard]] std::shared_ptr<ipcgull::node> node() const;

        void configure() const;

        bool pressed() const;

    private:
        friend class ButtonWrapper;

        Button(Info info, int index,
               Device* device, ConfigFunction conf_func,
               const std::shared_ptr<ipcgull::node>& root,
               config::Button& config);

        class IPC : public ipcgull::interface {
        public:
            IPC(Button* parent,
                const Info& info);

            void setAction(const std::string& type);

        private:
            Button& _button;
        };

        std::shared_ptr<ipcgull::node> _node;
        std::shared_ptr<IPC> _interface;
        Device* _device;
        const ConfigFunction _conf_func;

        config::Button& _config;

        mutable std::mutex _action_lock;
        std::shared_ptr<actions::Action> _action;
        const Info _info;

        std::weak_ptr<Button> _self;
    };

    class RemapButton : public DeviceFeature {
    public:
        explicit RemapButton(Device* dev);

        ~RemapButton() noexcept override;

        void configure() final;

        void listen() final;

    private:
        void _buttonEvent(const std::set<uint16_t>& new_state);

        std::shared_ptr<backend::hidpp20::ReprogControls> _reprog_controls;
        std::set<uint16_t> _pressed_buttons;
        std::mutex _button_lock;

        std::optional<config::RemapButton>& _config;
        std::map<uint16_t, std::shared_ptr<Button>> _buttons;

        std::shared_ptr<ipcgull::node> _ipc_node;

        class IPC : public ipcgull::interface {
        public:
            explicit IPC(RemapButton* parent);

            [[nodiscard]] std::vector<std::shared_ptr<Button>> enumerate() const;

        private:
            RemapButton& _parent;
        };

        std::shared_ptr<IPC> _ipc_interface;
        std::optional<backend::hidpp::Device::EvHandlerId> _ev_handler;
    };
}

#endif //LOGID_FEATURE_REMAPBUTTON_H
