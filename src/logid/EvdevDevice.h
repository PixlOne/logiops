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