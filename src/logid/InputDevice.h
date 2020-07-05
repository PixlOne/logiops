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

#ifndef LOGID_INPUTDEVICE_H
#define LOGID_INPUTDEVICE_H

#include <memory>

extern "C"
{
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
};

namespace logid
{
    typedef uint keycode;

    class InputDevice
    {
    public:
        class InvalidEventCode : public std::exception
        {
        public:
            explicit InvalidEventCode(std::string name);
            const char* what() const noexcept override;
        private:
            std::string _what;
        };
        explicit InputDevice(const char *name);
        ~InputDevice();

        void moveAxis(uint axis, int movement);
        void pressKey(uint code);
        void releaseKey(uint code);

        static uint toKeyCode(std::string name);
        static uint toAxisCode(std::string name);
    private:
        void _sendEvent(uint type, uint code, int value);

        static uint _toEventCode(uint type, const std::string& name);

        libevdev* device;
        libevdev_uinput* ui_device;
    };

    extern std::unique_ptr<InputDevice> virtual_input;
}

#endif //LOGID_INPUTDEVICE_H