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
#include "KeypressAction.h"
#include "../util/log.h"
#include "../InputDevice.h"
#include "../backend/hidpp20/features/ReprogControls.h"

using namespace logid::actions;
using namespace logid::backend;

KeypressAction::KeypressAction(Device *device, libconfig::Setting& config) :
    Action(device), _config (device, config)
{
}

void KeypressAction::press()
{
    _pressed = true;
    for(auto& key : _config.keys())
        virtual_input->pressKey(key);
}

void KeypressAction::release()
{
    _pressed = false;
    for(auto& key : _config.keys())
        virtual_input->releaseKey(key);
}

uint8_t KeypressAction::reprogFlags() const
{
    return hidpp20::ReprogControls::TemporaryDiverted;
}

KeypressAction::Config::Config(Device* device, libconfig::Setting& config) :
    Action::Config(device)
{
    if(!config.isGroup()) {
        logPrintf(WARN, "Line %d: action must be an object, skipping.",
                config.getSourceLine());
        return;
    }

    try {
        auto &keys = config.lookup("keys");
        if(keys.isArray() || keys.isList()) {
            int key_count = keys.getLength();
            for(int i = 0; i < key_count; i++) {
                auto& key = keys[i];
                if(key.isNumber()) {
                    _keys.push_back(key);
                    virtual_input->registerKey(key);
                } else if(key.getType() == libconfig::Setting::TypeString) {
                    try {
                        _keys.push_back(virtual_input->toKeyCode(key));
                        virtual_input->registerKey(virtual_input->toKeyCode(key));
                    } catch(InputDevice::InvalidEventCode& e) {
                        logPrintf(WARN, "Line %d: Invalid keycode %s, skipping."
                            , key.getSourceLine(), key.c_str());
                    }
                } else {
                    logPrintf(WARN, "Line %d: keycode must be string or int",
                            key.getSourceLine(), key.c_str());
                }
            }
        }
    } catch (libconfig::SettingNotFoundException& e) {
        logPrintf(WARN, "Line %d: keys is a required field, skipping.",
                config.getSourceLine());
    }
}

std::vector<unsigned int>& KeypressAction::Config::keys()
{
    return _keys;
}
