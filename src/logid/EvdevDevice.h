#ifndef MASTEROPTIONS_EVDEVDEVICE_H
#define MASTEROPTIONS_EVDEVDEVICE_H

#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

class EvdevDevice
{
public:
    EvdevDevice(const char* name);
    ~EvdevDevice();
    void send_event(unsigned int type, unsigned int code, int value);
    libevdev* device;
    libevdev_uinput* ui_device;
};

extern EvdevDevice* global_evdev;

#endif //MASTEROPTIONS_EVDEVDEVICE_H
