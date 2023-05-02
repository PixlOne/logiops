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
#ifndef LOGID_ACTION_H
#define LOGID_ACTION_H

#include <atomic>
#include <memory>
#include <utility>
#include <shared_mutex>
#include <ipcgull/node.h>
#include <ipcgull/interface.h>
#include <config/schema.h>

namespace logid {
    class Device;
}

namespace logid::actions {
    class InvalidAction : public std::exception {
    public:
        InvalidAction() = default;

        InvalidAction(std::string action) : _action(std::move(action)) {}

        [[nodiscard]] const char* what() const noexcept override {
            return _action.c_str();
        }

    private:
        std::string _action;
    };

    class Action : public ipcgull::interface {
    public:
        static std::shared_ptr<Action> makeAction(
                Device* device, const std::string& name,
                std::optional<config::BasicAction>& config,
                const std::shared_ptr<ipcgull::node>& parent);

        static std::shared_ptr<Action> makeAction(
                Device* device, const std::string& name,
                std::optional<config::Action>& config,
                const std::shared_ptr<ipcgull::node>& parent);

        static std::shared_ptr<Action> makeAction(
                Device* device, config::BasicAction& action,
                const std::shared_ptr<ipcgull::node>& parent);

        static std::shared_ptr<Action> makeAction(
                Device* device, config::Action& action,
                const std::shared_ptr<ipcgull::node>& parent);

        virtual void press() = 0;

        virtual void release() = 0;

        virtual void move([[maybe_unused]] int16_t x, [[maybe_unused]] int16_t y) { }

        virtual bool pressed() {
            return _pressed;
        }

        [[nodiscard]] virtual uint8_t reprogFlags() const = 0;

        virtual ~Action() = default;

    protected:
        Action(Device* device, const std::string& name, tables t = {});

        Device* _device;
        std::atomic<bool> _pressed;
        mutable std::shared_mutex _config_mutex;

        template <typename T>
        [[nodiscard]] std::weak_ptr<T> self() const {
            return std::dynamic_pointer_cast<T>(_self.lock());
        }

    private:
        std::weak_ptr<Action> _self;
    };
}

#endif //LOGID_ACTION_H
