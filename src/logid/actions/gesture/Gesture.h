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
#ifndef LOGID_ACTION_GESTURE_H
#define LOGID_ACTION_GESTURE_H

#include <utility>
#include <actions/Action.h>

namespace logid::actions {
    class InvalidGesture : public std::exception {
    public:
        explicit InvalidGesture(std::string what = "") : _what(std::move(what)) {
        }

        [[nodiscard]] const char* what() const noexcept override {
            return _what.c_str();
        }

    private:
        std::string _what;
    };

    class Gesture : public ipcgull::interface {
    public:
        virtual void press(bool init_threshold) = 0;

        virtual void release(bool primary) = 0;

        virtual void move(int16_t axis) = 0;

        [[nodiscard]] virtual bool wheelCompatibility() const = 0;

        [[nodiscard]] virtual bool metThreshold() const = 0;

        virtual ~Gesture() = default;

        static std::shared_ptr<Gesture> makeGesture(Device* device,
                                                    config::Gesture& gesture,
                                                    const std::shared_ptr<ipcgull::node>& parent);

        static std::shared_ptr<Gesture> makeGesture(
                Device* device, const std::string& type,
                config::Gesture& gesture,
                const std::shared_ptr<ipcgull::node>& parent);

    protected:
        Gesture(Device* device,
                std::shared_ptr<ipcgull::node> parent,
                const std::string& name, tables t = {});

        mutable std::shared_mutex _config_mutex;

        const std::shared_ptr<ipcgull::node> _node;
        Device* _device;
    };
}

#endif //LOGID_ACTION_GESTURE_H
