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

#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <system_error>

#include "EvdevDevice.h"

using namespace logid;

EvdevDevice::EvdevDevice(const char* name)
{
    device = libevdev_new();
    libevdev_set_name(device, name);

    libevdev_enable_event_type(device, EV_KEY);
    for(int i = 0; i < KEY_CNT; i++)
        libevdev_enable_event_code(device, EV_KEY, i, nullptr);
    libevdev_enable_event_type(device, EV_REL);
    for(int i = 0; i < REL_CNT; i++)
        libevdev_enable_event_code(device, EV_REL, i, nullptr);

    int err = libevdev_uinput_create_from_device(device, LIBEVDEV_UINPUT_OPEN_MANAGED, &ui_device);

    if(err != 0)
        throw std::system_error(-err, std::generic_category());
}

void EvdevDevice::moveAxis(unsigned int axis, int movement)
{
    libevdev_uinput_write_event(ui_device, EV_REL, axis, movement);
    libevdev_uinput_write_event(ui_device, EV_SYN, SYN_REPORT, 0);
}

void EvdevDevice::sendEvent(unsigned int type, unsigned int code, int value)
{
    libevdev_uinput_write_event(ui_device, type, code, value);
    libevdev_uinput_write_event(ui_device, EV_SYN, SYN_REPORT, 0);
}

EvdevDevice::~EvdevDevice()
{
    libevdev_uinput_destroy(ui_device);
    libevdev_free(device);
}
