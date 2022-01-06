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
#ifndef LOGID_ACTION_H
#define LOGID_ACTION_H

#include <atomic>
#include <libconfig.h++>
#include <memory>

namespace logid {
    class Device;
namespace actions {
    class InvalidAction : public std::exception
    {
    public:
        InvalidAction()
        {
        }
        explicit InvalidAction(std::string& action) : _action (action)
        {
        }
        const char* what() const noexcept override
        {
            return _action.c_str();
        }
    private:
        std::string _action;
    };

    class Action
    {
    public:
        static std::shared_ptr<Action> makeAction(Device* device,
                libconfig::Setting& setting);

        virtual void press() = 0;
        virtual void release() = 0;
        virtual void move(int16_t x, int16_t y)
        {
            // Suppress unused warning
            (void)x; (void)y;
        }

        virtual bool pressed()
        {
            return _pressed;
        }

        virtual uint8_t reprogFlags() const = 0;

        virtual ~Action() = default;

        class Config
        {
        protected:
            explicit Config(Device* device) : _device (device)
            {
            }
            Device* _device;
        };
    protected:
        explicit Action(Device* device) : _device (device), _pressed (false)
        {
        }
        Device* _device;
        std::atomic<bool> _pressed;
    };
}}

#endif //LOGID_ACTION_H
