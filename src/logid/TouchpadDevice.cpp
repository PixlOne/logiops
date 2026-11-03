#include <TouchpadDevice.h>
#include <system_error>
#include <algorithm>

extern "C"
{
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <linux/input.h>
}

using namespace logid;

namespace {
    int clamp_value(int value, int min_value, int max_value) {
        return std::max(min_value, std::min(value, max_value));
    }
}

TouchpadDevice::TouchpadDevice(const char* name, int slots) :
        _name(name),
        _max_slots(slots > 0 ? slots : default_slots),
        _active(false),
        _active_fingers(0),
        _next_tracking_id(1) {
    _createDevice();
}

TouchpadDevice::~TouchpadDevice() {
    _destroyDevice();
}

int TouchpadDevice::slots() const {
    return _max_slots;
}

void TouchpadDevice::beginContacts(int fingers) {
    std::lock_guard<std::mutex> lock(_input_mutex);
    if (fingers < 1)
        return;

    if (fingers > _max_slots)
        fingers = _max_slots;

    _endContactsLocked();
    _active = true;
    _active_fingers = fingers;
    _contacts.clear();
    _contacts.reserve(fingers);

    const int x = range / 2;
    const int y = range / 2;

    for (int slot = 0; slot < fingers; ++slot) {
        if (_next_tracking_id > 65535)
            _next_tracking_id = 1;
        Contact contact{ x, y, _next_tracking_id++ };
        _contacts.emplace_back(contact);
        _sendEvent(EV_ABS, ABS_MT_SLOT, slot);
        _sendEvent(EV_ABS, ABS_MT_TRACKING_ID, contact.tracking_id);
        _sendEvent(EV_ABS, ABS_MT_POSITION_X, contact.x);
        _sendEvent(EV_ABS, ABS_MT_POSITION_Y, contact.y);
    }

    _sendEvent(EV_ABS, ABS_X, x);
    _sendEvent(EV_ABS, ABS_Y, y);
    _setToolButtons(fingers, true);
    _sendSyn();
}

void TouchpadDevice::moveContacts(int dx, int dy) {
    std::lock_guard<std::mutex> lock(_input_mutex);
    if (!_active || _contacts.empty())
        return;

    for (std::size_t slot = 0; slot < _contacts.size(); ++slot) {
        auto& contact = _contacts[slot];
        contact.x = clamp_value(contact.x + dx, 0, range);
        contact.y = clamp_value(contact.y + dy, 0, range);
        _sendEvent(EV_ABS, ABS_MT_SLOT, static_cast<int>(slot));
        _sendEvent(EV_ABS, ABS_MT_POSITION_X, contact.x);
        _sendEvent(EV_ABS, ABS_MT_POSITION_Y, contact.y);
    }

    _sendEvent(EV_ABS, ABS_X, _contacts.front().x);
    _sendEvent(EV_ABS, ABS_Y, _contacts.front().y);
    _sendSyn();
}

void TouchpadDevice::endContacts() {
    std::lock_guard<std::mutex> lock(_input_mutex);
    _endContactsLocked();
}

void TouchpadDevice::_createDevice() {
    _device = libevdev_new();
    libevdev_set_name(_device, _name.c_str());

    libevdev_enable_event_type(_device, EV_KEY);
    libevdev_enable_event_code(_device, EV_KEY, BTN_TOOL_FINGER, nullptr);
    libevdev_enable_event_code(_device, EV_KEY, BTN_TOOL_DOUBLETAP, nullptr);
    libevdev_enable_event_code(_device, EV_KEY, BTN_TOOL_TRIPLETAP, nullptr);
#ifdef BTN_TOOL_QUADTAP
    libevdev_enable_event_code(_device, EV_KEY, BTN_TOOL_QUADTAP, nullptr);
#endif
#ifdef BTN_TOOL_QUINTTAP
    libevdev_enable_event_code(_device, EV_KEY, BTN_TOOL_QUINTTAP, nullptr);
#endif
    libevdev_enable_event_code(_device, EV_KEY, BTN_TOUCH, nullptr);
    libevdev_enable_event_code(_device, EV_KEY, BTN_LEFT, nullptr);
    libevdev_enable_property(_device, INPUT_PROP_POINTER);

    libevdev_enable_event_type(_device, EV_ABS);

    input_absinfo abs_pos{};
    abs_pos.minimum = 0;
    abs_pos.maximum = range;
    abs_pos.value = range / 2;
    abs_pos.resolution = resolution;
    libevdev_enable_event_code(_device, EV_ABS, ABS_X, &abs_pos);
    libevdev_enable_event_code(_device, EV_ABS, ABS_Y, &abs_pos);

    input_absinfo mt_slot{};
    mt_slot.minimum = 0;
    mt_slot.maximum = _max_slots - 1;
    libevdev_enable_event_code(_device, EV_ABS, ABS_MT_SLOT, &mt_slot);

    input_absinfo mt_tracking{};
    mt_tracking.minimum = -1;
    mt_tracking.maximum = 65535;
    libevdev_enable_event_code(_device, EV_ABS, ABS_MT_TRACKING_ID, &mt_tracking);

    libevdev_enable_event_code(_device, EV_ABS, ABS_MT_POSITION_X, &abs_pos);
    libevdev_enable_event_code(_device, EV_ABS, ABS_MT_POSITION_Y, &abs_pos);

    int err = libevdev_uinput_create_from_device(_device,
                                                 LIBEVDEV_UINPUT_OPEN_MANAGED,
                                                 &_ui_device);
    if (err != 0) {
        libevdev_free(_device);
        _device = nullptr;
        throw std::system_error(-err, std::generic_category());
    }
}

void TouchpadDevice::_destroyDevice() {
    if (_ui_device) {
        libevdev_uinput_destroy(_ui_device);
        _ui_device = nullptr;
    }
    if (_device) {
        libevdev_free(_device);
        _device = nullptr;
    }
}

void TouchpadDevice::_sendEvent(uint type, uint code, int value) {
    libevdev_uinput_write_event(_ui_device, type, code, value);
}

void TouchpadDevice::_sendSyn() {
    libevdev_uinput_write_event(_ui_device, EV_SYN, SYN_REPORT, 0);
}

void TouchpadDevice::_setToolButtons(int fingers, bool down) {
    const int pressed = down ? 1 : 0;
    _sendEvent(EV_KEY, BTN_TOUCH, pressed);
    const bool one = down && fingers == 1;
    const bool two = down && fingers == 2;
    const bool three = down && fingers == 3;
    const bool four = down && fingers == 4;
    const bool five = down && fingers >= 5;
    _sendEvent(EV_KEY, BTN_TOOL_FINGER, one ? 1 : 0);
    _sendEvent(EV_KEY, BTN_TOOL_DOUBLETAP, two ? 1 : 0);
    _sendEvent(EV_KEY, BTN_TOOL_TRIPLETAP, three ? 1 : 0);
#ifdef BTN_TOOL_QUADTAP
    _sendEvent(EV_KEY, BTN_TOOL_QUADTAP, four ? 1 : 0);
#endif
#ifdef BTN_TOOL_QUINTTAP
    _sendEvent(EV_KEY, BTN_TOOL_QUINTTAP, five ? 1 : 0);
#endif
}

void TouchpadDevice::_endContactsLocked() {
    if (!_active || _contacts.empty()) {
        _active = false;
        _contacts.clear();
        _active_fingers = 0;
        return;
    }

    for (std::size_t slot = 0; slot < _contacts.size(); ++slot) {
        _sendEvent(EV_ABS, ABS_MT_SLOT, static_cast<int>(slot));
        _sendEvent(EV_ABS, ABS_MT_TRACKING_ID, -1);
    }
    _setToolButtons(_active_fingers, false);
    _sendSyn();
    _contacts.clear();
    _active = false;
    _active_fingers = 0;
}
