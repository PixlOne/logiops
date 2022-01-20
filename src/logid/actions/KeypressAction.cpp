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
#include "../Device.h"
#include "../util/log.h"
#include "../InputDevice.h"
#include "../backend/hidpp20/features/ReprogControls.h"

using namespace logid::actions;
using namespace logid::backend;

KeypressAction::KeypressAction(Device *device, config::KeypressAction& config) :
    Action(device), _config (config)
{
    if(std::holds_alternative<std::string>(_config.keys)) {
        const auto& key = std::get<std::string>(_config.keys);
        try {
            auto code = _device->virtualInput()->toKeyCode(key);
            _device->virtualInput()->registerKey(code);
            _keys.emplace_back(code);
        } catch(InputDevice::InvalidEventCode& e) {
            logPrintf(WARN, "Invalid keycode %s, skipping.", key.c_str());
        }
    } else if(std::holds_alternative<uint>(_config.keys)) {
        const auto& key = std::get<uint>(_config.keys);
        _device->virtualInput()->registerKey(key);
        _keys.emplace_back(key);
    } else if(std::holds_alternative<std::list<std::variant<uint, std::string
            >>>(_config.keys)) {
        const auto& keys = std::get<std::list<std::variant<uint, std::string>>>(
                _config.keys);
        for(const auto& key : keys) {
            if(std::holds_alternative<std::string>(_config.keys)) {
                const auto& key_str = std::get<std::string>(key);
                try {
                    auto code = _device->virtualInput()->toKeyCode(key_str);
                    _device->virtualInput()->registerKey(code);
                    _keys.emplace_back(code);
                } catch(InputDevice::InvalidEventCode& e) {
                    logPrintf(WARN, "Invalid keycode %s, skipping.",
                              key_str.c_str());
                }
            } else if(std::holds_alternative<uint>(_config.keys)) {
                auto& code = std::get<uint>(_config.keys);
                _device->virtualInput()->registerKey(code);
                _keys.emplace_back(code);
            }
        }
    }
}

void KeypressAction::press()
{
    _pressed = true;
    for(auto& key : _keys)
        _device->virtualInput()->pressKey(key);
}

void KeypressAction::release()
{
    _pressed = false;
    for(auto& key : _keys)
        _device->virtualInput()->releaseKey(key);
}

uint8_t KeypressAction::reprogFlags() const
{
    return hidpp20::ReprogControls::TemporaryDiverted;
}
