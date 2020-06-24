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

#ifndef LOGID_EVDEVDEVICE_H
#define LOGID_EVDEVDEVICE_H

#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

namespace logid
{
    class EvdevDevice
    {
    public:
        EvdevDevice(const char *name);

        ~EvdevDevice();

        void moveAxis(unsigned int axis, int movement);

        void sendEvent(unsigned int type, unsigned int code, int value);

        libevdev *device;
        libevdev_uinput *ui_device;
    };

    extern EvdevDevice* global_evdev;
}

#endif //LOGID_EVDEVDEVICE_H