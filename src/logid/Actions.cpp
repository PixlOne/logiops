#include <hidpp20/Error.h>
#include <hidpp/SimpleDispatcher.h>
#include <hidpp20/IAdjustableDPI.h>

#include "Actions.h"
#include "util.h"
#include "EvdevDevice.h"

KeyAction::KeyAction(const KeyAction &a, Device* d) : ButtonAction(Action::Keypress)
{
    device = d;
    std::copy(a.keys.begin(), a.keys.end(), std::back_inserter(keys));
}

void KeyAction::press()
{
    //KeyPress event for each in keys
    for(unsigned int i : keys)
        global_evdev->send_event(EV_KEY, i, 1);
}

void KeyAction::release()
{
    //KeyRelease event for each in keys
    for(unsigned int i : keys)
        global_evdev->send_event(EV_KEY, i, 0);
}

void GestureAction::press()
{
    held = true;
    x = 0;
    y = 0;
}

void GestureAction::move(HIDPP20::IReprogControlsV4::Move m)
{
    x += m.x;
    y += m.y;
}

void GestureAction::release()
{
    held = false;
    auto direction = get_direction(x, y);

    auto g = gestures.find(direction);

    if(g != gestures.end() && g->second->mode == GestureMode::OnRelease)
    {
        g->second->action->press();
        g->second->action->release();
    }
}

void SmartshiftAction::press()
{
    if(device->features.find(0x2110) == device->features.end())
    {
        log_printf(DEBUG, "Error toggling smart shift, feature is non-existent.");
        return;
    }
    const uint8_t f_index = device->features.find(0x2110)->second;
    std::vector<uint8_t> results;

    try
    {
        results = device->hidpp_dev->callFunction(f_index, 0x00);
        if(results[0] == 0x02)
            results = device->hidpp_dev->callFunction(f_index, 0x01, {0x01});
        else if(results[0] == 0x01)
            results = device->hidpp_dev->callFunction(f_index, 0x01, {0x02});
        else
            results = device->hidpp_dev->callFunction(f_index, 0x01, {0x01});
    }
    catch(HIDPP20::Error &e)
    {
        log_printf(ERROR, "Error toggling smart shift, code %d: %s\n", e.errorCode(), e.what());
        return;
    }
}

void HiresScrollAction::press()
{
    if(device->features.find(0x2110) == device->features.end())
    {
        log_printf(DEBUG, "Error toggling hires scroll, feature is non-existent.");
        return;
    }
    const uint8_t f_index = device->features.find(0x2121)->second;
    std::vector<uint8_t> results;

    try
    {
        results = device->hidpp_dev->callFunction(f_index, 0x01);
        if(results[0] == 0x02)
            results = device->hidpp_dev->callFunction(f_index, 0x02, {0x00});
        else
            results = device->hidpp_dev->callFunction(f_index, 0x02, {0x02});
    }
    catch(HIDPP20::Error &e)
    {
        log_printf(ERROR, "Error toggling hires scroll, code %d: %s\n", e.errorCode(), e.what());
        return;
    }
}

void CycleDPIAction::press()
{
    HIDPP20::IAdjustableDPI iad(device->hidpp_dev);

    try
    {
        for (uint i = 0; i < iad.getSensorCount(); i++)
        {
            int current_dpi = std::get<0>(iad.getSensorDPI(i));
            bool found = false;
            for (uint j = 0; j < dpis.size(); j++)
            {
                if (dpis[j] == current_dpi)
                {
                    if (j == dpis.size() - 1)
                        iad.setSensorDPI(i, dpis[0]);
                    else
                        iad.setSensorDPI(i, dpis[j + 1]);
                    found = true;
                    break;
                }
            }
            if (found) break;
            if (dpis.empty()) iad.setSensorDPI(i, std::get<1>(iad.getSensorDPI(i)));
            else iad.setSensorDPI(i, dpis[0]);
        }
    }
    catch(HIDPP20::Error &e) { log_printf(ERROR, "Error while cycling DPI: %s", e.what()); }
}

void ChangeDPIAction::press()
{
    HIDPP20::IAdjustableDPI iad(device->hidpp_dev);

    try
    {
        for(uint i = 0; i < iad.getSensorCount(); i++)
        {
            int current_dpi = std::get<0>(iad.getSensorDPI(i));
            iad.setSensorDPI(i, current_dpi + dpi_inc);
        }
    }
    catch(HIDPP20::Error &e)
    {
        log_printf(ERROR, "Error while incrementing DPI: %s", e.what());
    }
}