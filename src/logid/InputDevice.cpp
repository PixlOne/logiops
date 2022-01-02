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

#include <system_error>

#include "InputDevice.h"

extern "C"
{
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
}

using namespace logid;

InputDevice::InvalidEventCode::InvalidEventCode(const std::string& name) :
        _what ("Invalid event code " + name)
{
}

const char* InputDevice::InvalidEventCode::what() const noexcept
{
    return _what.c_str();
}

InputDevice::InputDevice(const char* name)
{
    device = libevdev_new();
    libevdev_set_name(device, name);

    libevdev_enable_event_type(device, EV_KEY);
    for (unsigned int i = 0; i < KEY_CNT; i++) {
        // Enable some keys which a normal keyboard should have
        // by default, i.e. a-z, modifier keys and so on, see:
        // /usr/include/linux/input-event-codes.h
        if (i < 128) {
            registered_keys[i] = true;
            libevdev_enable_event_code(device, EV_KEY, i, nullptr);
        } else {
            registered_keys[i] = false;
        }
    }

    libevdev_enable_event_type(device, EV_REL);

    int err = libevdev_uinput_create_from_device(device,
            LIBEVDEV_UINPUT_OPEN_MANAGED, &ui_device);

    if(err != 0) {
        libevdev_free(device);
        throw std::system_error(-err, std::generic_category());
    }
}

InputDevice::~InputDevice()
{
    libevdev_uinput_destroy(ui_device);
    libevdev_free(device);
}

void InputDevice::registerKey(uint code)
{
    // TODO: Maybe print error message, if wrong code is passed?
    if(registered_keys[code] || code > KEY_CNT) {
        return;
    }

    _enableEvent(EV_KEY, code);

    registered_keys[code] = true;
}

void InputDevice::registerAxis(uint axis)
{
    // TODO: Maybe print error message, if wrong code is passed?
    if(registered_axis[axis] || axis > REL_CNT) {
        return;
    }

    _enableEvent(EV_REL, axis);

    registered_axis[axis] = true;
}

void InputDevice::moveAxis(uint axis, int movement)
{
    _sendEvent(EV_REL, axis, movement);
}

void InputDevice::pressKey(uint code)
{
    _sendEvent(EV_KEY, code, 1);
}

void InputDevice::releaseKey(uint code)
{
    _sendEvent(EV_KEY, code, 0);
}

uint InputDevice::toKeyCode(const std::string& name)
{
    return _toEventCode(EV_KEY, name);
}

uint InputDevice::toAxisCode(const std::string& name)
{
    return _toEventCode(EV_REL, name);
}

/* Returns -1 if axis_code is not hi-res */
int InputDevice::getLowResAxis(const uint axis_code)
{
    /* Some systems don't have these hi-res axes */
#ifdef REL_WHEEL_HI_RES
    if(axis_code == REL_WHEEL_HI_RES)
        return REL_WHEEL;
#endif
#ifdef REL_HWHEEL_HI_RES
    if(axis_code == REL_HWHEEL_HI_RES)
        return REL_HWHEEL;
#endif

    return -1;
}

uint InputDevice::_toEventCode(uint type, const std::string& name)
{
    int code = libevdev_event_code_from_name(type, name.c_str());

    if(code == -1)
        throw InvalidEventCode(name);

    return code;
}

void InputDevice::_enableEvent(const uint type, const uint code)
{
    libevdev_uinput_destroy(ui_device);

    libevdev_enable_event_code(device, type, code, nullptr);

    int err = libevdev_uinput_create_from_device(device,
            LIBEVDEV_UINPUT_OPEN_MANAGED, &ui_device);

    if(err != 0) {
        libevdev_free(device);
        device = nullptr;
        ui_device = nullptr;
        throw std::system_error(-err, std::generic_category());
    }
}

void InputDevice::_sendEvent(uint type, uint code, int value)
{
    libevdev_uinput_write_event(ui_device, type, code, value);
    libevdev_uinput_write_event(ui_device, EV_SYN, SYN_REPORT, 0);
}
