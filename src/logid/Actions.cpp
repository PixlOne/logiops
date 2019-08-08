#include <unistd.h>
#include <future>
#include <hidpp20/Error.h>
#include <hidpp/SimpleDispatcher.h>
#include <hidpp20/IAdjustableDPI.h>
#include <hidpp20/ISmartShift.h>
#include <hidpp20/IHiresScroll.h>
#include <cmath>

#include "Actions.h"
#include "util.h"
#include "EvdevDevice.h"

NoAction* NoAction::copy(Device *dev)
{
    auto action = new NoAction();
    action->device = dev;
    return action;
}
KeyAction* KeyAction::copy(Device *dev)
{
    auto action = new KeyAction(keys);
    action->device = dev;
    return action;
}
GestureAction* GestureAction::copy(Device* dev)
{
    auto action = new GestureAction({});
    action->device = dev;
    for(auto it : gestures)
        action->gestures.insert({it.first, new Gesture(*it.second, dev)});
    return action;
}
SmartshiftAction* SmartshiftAction::copy(Device* dev)
{
    auto action = new SmartshiftAction();
    action->device = dev;
    return action;
}
HiresScrollAction* HiresScrollAction::copy(Device* dev)
{
    auto action = new HiresScrollAction();
    action->device = dev;
    return action;
}
CycleDPIAction* CycleDPIAction::copy(Device* dev)
{
    auto action = new CycleDPIAction(dpis);
    action->device = dev;
    return action;
}
ChangeDPIAction* ChangeDPIAction::copy(Device* dev)
{
    auto action = new ChangeDPIAction(dpi_inc);
    action->device = dev;
    return action;
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
    if(m.y != 0 && abs(y) > 50)
    {
        auto g = gestures.find(m.y > 0 ? Direction::Down : Direction::Up);
        if(g != gestures.end())
        {
            if (g->second->mode == GestureMode::Axis)
                global_evdev->move_axis(g->second->axis, abs(m.y) * g->second->axis_multiplier);
            if (g->second->mode == GestureMode::OnFewPixels)
            {
                g->second->per_pixel_mod += abs(m.y);
                if(g->second->per_pixel_mod >= g->second->per_pixel)
                {
                    g->second->per_pixel_mod -= g->second->per_pixel;
                    g->second->action->press();
                    g->second->action->release();
                }
            }
        }
    }
    if(m.x != 0 && abs(x) > 50)
    {
        auto g = gestures.find(m.x > 0 ? Direction::Right : Direction::Left);
        if(g != gestures.end())
        {
            if (g->second->mode == GestureMode::Axis)
                global_evdev->move_axis(g->second->axis, abs(m.x) * g->second->axis_multiplier);
            if (g->second->mode == GestureMode::OnFewPixels)
            {
                g->second->per_pixel_mod += abs(m.x);
                if (g->second->per_pixel_mod >= g->second->per_pixel)
                {
                    g->second->per_pixel_mod -= g->second->per_pixel;
                    g->second->action->press();
                    g->second->action->release();
                }
            }
        }
    }
}

void GestureAction::release()
{
    held = false;
    Direction direction;
    if(abs(x) < 50 && abs(y) < 50) direction = Direction::None;
    else direction = get_direction(x, y);
    x = 0;
    y = 0;

    auto g = gestures.find(direction);

    if(g != gestures.end() && g->second->mode == GestureMode::OnRelease)
    {
        g->second->action->press();
        g->second->action->release();
    }
}

void SmartshiftAction::press()
{
    try
    {
        HIDPP20::ISmartShift iss(device->hidpp_dev);
        auto s = iss.getStatus();
        iss.setStatus({new bool(!*s.Active)});
    }
    catch(HIDPP20::Error &e)
    {
        log_printf(ERROR, "Error toggling smart shift, code %d: %s\n", e.errorCode(), e.what());
    }
}

void HiresScrollAction::press()
{
    try
    {
        HIDPP20::IHiresScroll ihs(device->hidpp_dev);
        auto mode = ihs.getMode();
        mode ^= HIDPP20::IHiresScroll::Mode::HiRes;
        ihs.setMode(mode);
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