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

#include <InputDevice.h>
#include <system_error>
#include <mutex>

extern "C"
{
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
}

using namespace logid;

InputDevice::InvalidEventCode::InvalidEventCode(const std::string& name) :
        _what("Invalid event code " + name) {
}

InputDevice::InvalidEventCode::InvalidEventCode(uint code) :
        _what("Invalid event code " + std::to_string(code)) {
}

const char* InputDevice::InvalidEventCode::what() const noexcept {
    return _what.c_str();
}

InputDevice::InputDevice(const char* name) {
    device = libevdev_new();
    libevdev_set_name(device, name);

    /* Enable every valid EV_KEY and EV_REL code up-front so the uinput
     * node never has to be destroyed and recreated when a new key or
     * axis is requested at runtime. Recreating exposed a window where
     * ui_device could be left null on failure, and changed the
     * /dev/input/eventN number under the feet of any application that
     * had the previous node open. */
    libevdev_enable_event_type(device, EV_KEY);
    for (unsigned int i = 0; i < KEY_CNT; i++) {
        if (libevdev_event_code_get_name(EV_KEY, i)) {
            registered_keys[i] = true;
            libevdev_enable_event_code(device, EV_KEY, i, nullptr);
        } else {
            registered_keys[i] = false;
        }
    }

    libevdev_enable_event_type(device, EV_REL);
    for (unsigned int i = 0; i < REL_CNT; i++) {
        if (libevdev_event_code_get_name(EV_REL, i)) {
            registered_axis[i] = true;
            libevdev_enable_event_code(device, EV_REL, i, nullptr);
        } else {
            registered_axis[i] = false;
        }
    }

    int err = libevdev_uinput_create_from_device(device,
                                                 LIBEVDEV_UINPUT_OPEN_MANAGED, &ui_device);

    if (err != 0) {
        libevdev_free(device);
        throw std::system_error(-err, std::generic_category());
    }
}

InputDevice::~InputDevice() {
    libevdev_uinput_destroy(ui_device);
    libevdev_free(device);
}

void InputDevice::registerKey(uint code) {
    /* All valid keys are pre-enabled in the constructor. */
    (void) code;
}

void InputDevice::registerAxis(uint axis) {
    /* All valid axes are pre-enabled in the constructor. */
    (void) axis;
}

void InputDevice::moveAxis(uint axis, int movement) {
    _sendEvent(EV_REL, axis, movement);
}

void InputDevice::pressKey(uint code) {
    _sendEvent(EV_KEY, code, 1);
}

void InputDevice::releaseKey(uint code) {
    _sendEvent(EV_KEY, code, 0);
}

std::string InputDevice::toKeyName(uint code) {
    return _toEventName(EV_KEY, code);
}

uint InputDevice::toKeyCode(const std::string& name) {
    return _toEventCode(EV_KEY, name);
}

std::string InputDevice::toAxisName(uint code) {
    return _toEventName(EV_REL, code);
}

uint InputDevice::toAxisCode(const std::string& name) {
    return _toEventCode(EV_REL, name);
}

/* Returns -1 if axis_code is not hi-res */
int InputDevice::getLowResAxis(const uint axis_code) {
    /* Some systems don't have these hi-res axes */
#ifdef REL_WHEEL_HI_RES
    if (axis_code == REL_WHEEL_HI_RES)
        return REL_WHEEL;
#endif
#ifdef REL_HWHEEL_HI_RES
    if (axis_code == REL_HWHEEL_HI_RES)
        return REL_HWHEEL;
#endif

    return -1;
}

std::string InputDevice::_toEventName(uint type, uint code) {
    const char* ret = libevdev_event_code_get_name(type, code);

    if (!ret)
        throw InvalidEventCode(code);

    return {ret};
}

uint InputDevice::_toEventCode(uint type, const std::string& name) {
    int code = libevdev_event_code_from_name(type, name.c_str());

    if (code == -1)
        throw InvalidEventCode(name);

    return code;
}

void InputDevice::_sendEvent(uint type, uint code, int value) {
    std::unique_lock lock(_input_mutex);
    if (!ui_device)
        return;
    libevdev_uinput_write_event(ui_device, type, code, value);
    libevdev_uinput_write_event(ui_device, EV_SYN, SYN_REPORT, 0);
}
