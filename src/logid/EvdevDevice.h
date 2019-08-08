#ifndef EVDEVDEVICE_H
#define EVDEVDEVICE_H

#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

class EvdevDevice
{
public:
    EvdevDevice(const char* name);
    ~EvdevDevice();
    void move_axis(unsigned int axis, int movement);
    void send_event(unsigned int type, unsigned int code, int value);
    libevdev* device;
    libevdev_uinput* ui_device;
};

extern EvdevDevice* global_evdev;

#endif //EVDEVDEVICE_H