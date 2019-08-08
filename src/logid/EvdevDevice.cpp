#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <system_error>

#include "EvdevDevice.h"

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

void EvdevDevice::move_axis(unsigned int axis, int movement)
{
    libevdev_uinput_write_event(ui_device, EV_REL, axis, movement);
    libevdev_uinput_write_event(ui_device, EV_SYN, SYN_REPORT, 0);
}

void EvdevDevice::send_event(unsigned int type, unsigned int code, int value)
{
    libevdev_uinput_write_event(ui_device, type, code, value);
    libevdev_uinput_write_event(ui_device, EV_SYN, SYN_REPORT, 0);
}

EvdevDevice::~EvdevDevice()
{
    libevdev_uinput_destroy(ui_device);
    libevdev_free(device);
}
