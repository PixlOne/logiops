#include <cstdint>
#include <vector>
#include <map>
#include <linux/input-event-codes.h>
#include <libevdev/libevdev.h>
#include <algorithm>
#include <cstring>
#include <hidpp20/IHiresScroll.h>

#include "Configuration.h"
#include "util.h"

using namespace logid;
using namespace libconfig;

Configuration::Configuration(const char *config_file)
{
    //Read config file
    try
    {
        cfg.readFile(config_file);
    }
    catch(const FileIOException &e)
    {
        log_printf(ERROR, "I/O Error while reading %s: %s", config_file, e.what());
        throw e;
    }
    catch(const ParseException &e)
    {
        log_printf(ERROR, "Parse error in %s, line %d: %s", e.getFile(), e.getLine(), e.getError());
        throw e;
    }
    const Setting &root = cfg.getRoot();
    Setting* _devices;

    try { _devices = &root["devices"]; }
    catch(const SettingNotFoundException &e)
    {
        log_printf(WARN, "No devices listed in config file.");
        return;
    }

    for(int i = 0; i < _devices->getLength(); i++)
    {
        const Setting &device = (*_devices)[i];
        std::string name;
        try
        {
            if(!device.lookupValue("name", name))
            {
                log_printf(WARN, "Line %d: 'name' must be a string, skipping device.", device["name"].getSourceLine());
                continue;
            }
        }
        catch(SettingNotFoundException &e)
        {
            log_printf(WARN, "Line %d: Missing 'name' field, skipping device.", device.getSourceLine());
            continue;
        }
        devices.insert({name, new DeviceConfig(device)});
    }
}

DeviceConfig::DeviceConfig(const libconfig::Setting &root)
{
    try
    {
        int d;
        if(!root.lookupValue("dpi", d))
            throw SettingTypeException(root["dpi"]);
        dpi = new int(d);
    }
    catch(const SettingNotFoundException &e) { }
    catch(const SettingTypeException &e)
    {
        log_printf(WARN, "Line %d: DPI must me an integer; not setting.", root["dpi"].getSourceLine());
    }

    try
    {
        const Setting& hr = root["hiresscroll"];
        uint8_t hss = 0;
        try
        {
            bool b;
            if(!hr.lookupValue("hires", b))
                throw SettingTypeException(root["hires"]);
            if(b) hss |= HIDPP20::IHiresScroll::HiRes;
        }
        catch(SettingNotFoundException &e) { }
        catch(SettingTypeException &e)
        {
            log_printf(INFO, "Line %d: hires field must be a boolean", hr["hires"].getSourceLine());
        }

        try
        {
            bool b;
            if(!hr.lookupValue("invert", b))
                throw SettingTypeException(root["invert"]);
            if(b) hss |= HIDPP20::IHiresScroll::Inverted;
        }
        catch(SettingNotFoundException &e) { }
        catch(SettingTypeException &e)
        {
            log_printf(INFO, "Line %d: invert field must be a boolean", hr["invert"].getSourceLine());
        }

        try
        {
            bool b;
            if(!hr.lookupValue("target", b))
                throw SettingTypeException(root["target"]);
            if(b) hss |= HIDPP20::IHiresScroll::Target;
        }
        catch(SettingNotFoundException &e) { }
        catch(SettingTypeException &e)
        {
            log_printf(INFO, "Line %d: target field must be a boolean", hr["target"].getSourceLine());
        }

        hiresscroll = new uint8_t(hss);
    }
    catch(const SettingNotFoundException &e)
    {
        log_printf(INFO, "Missing hiresscroll option, not setting.");
    }
    catch(const SettingTypeException &e)
    {
        log_printf(WARN, "Line %d: hiresscroll should be an object", root["hiresscroll"].getSourceLine());
    }

    try
    {
        const Setting& ss = root["smartshift"];
        smartshift = new HIDPP20::ISmartShift::SmartshiftStatus {};
        bool on;
        int threshold;
        try
        {
            if (ss.lookupValue("on", on)) smartshift->Active = new bool(on);
            else log_printf(WARN, "Line %d: on field must be a boolean", ss["on"].getSourceLine());
        }
        catch(const SettingNotFoundException &e) { }

        try
        {
            if (ss.lookupValue("threshold", threshold))
            {
                if(threshold < 0)
                {
                    threshold = 1;
                    log_printf(INFO, "Smartshift threshold must be > 0 or < 100, setting to 1.");
                }
                if(threshold >= 100)
                {
                    threshold = 99;
                    log_printf(INFO, "Smartshift threshold must be > 0 or < 100, setting to 99.");
                }
                smartshift->AutoDisengage = new uint8_t(threshold);
                smartshift->DefaultAutoDisengage = new uint8_t(threshold);
            }
            else log_printf(WARN, "Line %d: threshold must be an integer", ss["threshold"].getSourceLine());
        }
        catch(const SettingNotFoundException &e) { }
    }
    catch(const SettingNotFoundException &e) { }
    catch(const SettingTypeException &e)
    {
        log_printf(WARN, "Line %d: smartshift field must be an object", root["smartshift"].getSourceLine());
    }

    Setting* buttons;
    try
    {
        buttons = &root["buttons"];
    }
    catch(const SettingNotFoundException &e)
    {
        log_printf(WARN, "No button configuration found, reverting to null config.");
        new std::map<uint16_t, ButtonAction*>();
        return;
    }

    for(int i = 0; i < buttons->getLength(); i++)
    {
        const Setting &button = (*buttons)[i];

        int cid;
        try { button.lookupValue("cid", cid); }
        catch(SettingNotFoundException &e)
        {
            log_printf(WARN, "Entry on line %d is missing a cid", button.getSourceLine());
            continue;
        }

        if(actions.find(cid) != actions.end())
        {
            log_printf(WARN, "Duplicate entries for cid 0x%x, skipping entry on line %d", cid, button.getSourceLine());
            continue;
        }

        Setting* action_config;
        try { action_config = &button["action"]; }
        catch(SettingNotFoundException &e)
        {
            log_printf(WARN, "cid 0x%x is missing an action, not diverting!", cid);
            continue;
        }

        Action action_type;
        try
        {
            std::string action_type_str;
            action_config->lookupValue("type", action_type_str);
            action_type = stringToAction(action_type_str);
        }
        catch(SettingNotFoundException &e)
        {
            log_printf(WARN, "cid 0x%x is missing an action type, not diverting!", cid);
            continue;
        }
        catch(std::invalid_argument &e)
        {
            log_printf(WARN, "Line %d: %s", (*action_config)["type"].getSourceLine(), e.what());
            continue;
        }

        try { actions.insert({cid, parse_action(action_type, action_config)}); }
        catch(std::exception &e) { log_printf(ERROR, "%s", e.what()); }
    }
}

ButtonAction* logid::parse_action(Action type, const Setting* action_config, bool is_gesture)
{
    if(type == Action::None) return new NoAction();
    if(type == Action::Keypress)
    {
        std::vector<unsigned int> keys;
        try
        {
            const Setting &keys_config = (*action_config)["keys"];
            for (int i = 0; i < keys_config.getLength(); i++)
            {
                int keycode = libevdev_event_code_from_name(EV_KEY, keys_config[i]);
                if(keycode == -1)
                {
                    const char* keyname = keys_config[i];
                    log_printf(WARN, "%s is not a valid keycode, skipping", keyname);
                }
                else keys.push_back(keycode);
            }
        }
        catch(SettingNotFoundException &e)
        {
            log_printf(WARN, "Expected keys parameter on line %d", action_config->getSourceLine());
        }

        return new KeyAction(keys);
    }
    else if(type == Action::Gestures)
    {
        if(is_gesture)
        {
            log_printf(WARN, "Line %d: Recursive gesture, defaulting to no action.", action_config->getSourceLine());
            return new NoAction();
        }
        std::map<Direction, Gesture*> gestures;

        Setting* gestures_config;

        try { gestures_config = &(*action_config)["gestures"]; }
        catch(SettingNotFoundException &e)
        {
            log_printf(WARN, "No gestures parameter for line %d, skipping", action_config->getSourceLine());
            throw e;
        }

        for(int i = 0; i < gestures_config->getLength(); i++)
        {
            const Setting &gesture_config = (*gestures_config)[i];

            std::string direction_str;
            Direction direction;
            try
            {
                gesture_config.lookupValue("direction", direction_str);
                direction = stringToDirection(direction_str);
            }
            catch(SettingNotFoundException &e)
            {
                log_printf(WARN, "No direction set on line %d", gesture_config.getSourceLine());
                continue;
            }
            catch(std::invalid_argument &e)
            {
                log_printf(WARN, "Line %d: %s", gesture_config["direction"].getSourceLine(), e.what());
                continue;
            }

            if(gestures.find(direction) != gestures.end())
            {
                log_printf(WARN, "Entry on line %d is a duplicate, skipping...", gesture_config["direction"].getSourceLine());
                continue;
            }

            GestureMode mode;
            try
            {
                std::string mode_str;
                gesture_config.lookupValue("mode", mode_str);
                mode = stringToGestureMode(mode_str);
            }
            catch (SettingNotFoundException &e)
            {
                log_printf(INFO, "Gesture mode on line %d not found, defaulting to OnRelease", gesture_config.getSourceLine());
                mode = GestureMode::OnRelease;
            }

            if(mode == GestureMode::NoPress)
            {
                gestures.insert({direction, new Gesture(new NoAction(), mode)});
                continue;
            }

            if(mode == GestureMode::Axis)
            {
                Gesture::axis_info axis;
                try
                {
                    std::string axis_str;
                    if(!gesture_config.lookupValue("axis", axis_str))
                        throw SettingTypeException(gesture_config["axis"]);
                    axis.code = libevdev_event_code_from_name(EV_REL, axis_str.c_str());
                }
                catch(SettingNotFoundException &e)
                {
                    log_printf(WARN, "Line %d: No axis found, defaulting to no action.", gesture_config.getSourceLine());
                    gestures.insert({direction, new Gesture(new NoAction(), GestureMode::NoPress)});
                    continue;
                }
                catch(SettingTypeException &e)
                {
                    log_printf(WARN, "Line %d: Axis must be a string (e.g. 'REL_WHEEL')", gesture_config["axis"].getSourceLine());
                    gestures.insert({direction, new Gesture(new NoAction(), GestureMode::NoPress)});
                    continue;
                }

                axis.multiplier = 1;
                try
                {
                    if(!gesture_config.lookupValue("axis_multiplier", axis.multiplier))
                    {
                        int im = 1;
                        if(!gesture_config.lookupValue("axis_multiplier", im))
                            throw SettingTypeException(gesture_config["axis_multiplier"]);
                        axis.multiplier = (float)im;
                    }
                }
                catch(SettingNotFoundException &e) { }
                catch(SettingTypeException &e)
                {
                    log_printf(WARN, "Line %d: axis_multiplier must be a float/integer", gesture_config["axis_multiplier"].getSourceLine());
                    continue;
                }

                gestures.insert({direction, new Gesture(new NoAction(), GestureMode::Axis, &axis)});
                continue;
            }

            Setting* g_action;
            try { g_action = &gesture_config["action"]; }
            catch(SettingNotFoundException &e)
            {
                log_printf(WARN, "No action set for %s", direction_str.c_str());
                continue;
            }

            Action g_type;
            try
            {
                std::string type_str;
                g_action->lookupValue("type", type_str);
                g_type = stringToAction(type_str);
            }
            catch(SettingNotFoundException &e)
            {
                log_printf(INFO, "Missing an action type on line %d, skipping", g_action->getSourceLine());
                continue;
            }
            catch(std::invalid_argument &e)
            {
                log_printf(WARN, "Line %d: %s", (*g_action)["type"].getSourceLine(), e.what());
                continue;
            }

            ButtonAction* ba;

            try { ba = parse_action(g_type, g_action, true); }
            catch(std::exception &e) { continue; }

            if(mode == GestureMode::OnFewPixels)
            {
                try
                {
                    int pp;
                    if(!gesture_config.lookupValue("pixels", pp))
                        throw SettingTypeException(gesture_config["pixels"]);
                    gestures.insert({direction, new Gesture(ba, mode, &pp)});
                }
                catch(SettingNotFoundException &e)
                {
                    log_printf(WARN, "Line %d: OnFewPixels requires a 'pixels' field.", gesture_config.getSourceLine());
                }
                catch(SettingTypeException &e)
                {
                    log_printf(WARN, "Line %d: pixels must be an integer", gesture_config["pixels"].getSourceLine());
                    continue;
                }
            }
            else gestures.insert({direction, new Gesture(ba, mode)});
        }

        return new GestureAction(gestures);
    }
    else if(type == Action::ToggleSmartshift) return new SmartshiftAction();
    else if(type == Action::ToggleHiresScroll) return new HiresScrollAction();
    else if(type == Action::CycleDPI)
    {
        std::vector<int> dpis;
        try
        {
            const Setting &keys_config = (*action_config)["dpis"];
            for (int i = 0; i < keys_config.getLength(); i++)
            {
                dpis.push_back((int)keys_config[i]);
            }
        }
        catch(SettingNotFoundException &e)
        {
            log_printf(ERROR, "Line %d: CycleDPI action is missing 'dpis' field, defaulting to NoAction.", action_config->getSourceLine());
        }

        return new CycleDPIAction(dpis);
    }
    else if(type == Action::ChangeDPI)
    {
        int inc;
        try
        {
            action_config->lookupValue("inc", inc);
        }
        catch(SettingNotFoundException &e)
        {
            log_printf(ERROR, "Line %d: ChangeDPI action is missing an 'inc' field, defaulting to NoAction.",action_config->getSourceLine());
            return new NoAction();
        }

        return new ChangeDPIAction(inc);
    }

    log_printf(ERROR, "This shouldn't have happened. Unhandled action type? Defaulting to NoAction");
    return new NoAction();
}

DeviceConfig::DeviceConfig(DeviceConfig* dc, Device* dev) : baseConfig (false)
{
    dpi = dc->dpi;
    smartshift = dc->smartshift;
    hiresscroll = dc->hiresscroll;
    for(auto it : dc->actions)
        actions.insert( { it.first, it.second->copy(dev) } );
}

DeviceConfig::DeviceConfig()
{
    dpi = nullptr;
    hiresscroll = nullptr;
    smartshift = nullptr;
    actions = {};
}

DeviceConfig::~DeviceConfig()
{
    for(auto it : this->actions)
        delete(it.second);
}