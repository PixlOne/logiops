/*
 * Copyright 2019-2023 PixlOne
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
#include <actions/ChangeProfile.h>
#include <backend/hidpp20/features/ReprogControls.h>
#include <Device.h>

using namespace logid;
using namespace logid::actions;

const char* ChangeProfile::interface_name = "ChangeProfile";

ChangeProfile::ChangeProfile(Device* device, config::ChangeProfile& config,
                             [[maybe_unused]] const std::shared_ptr<ipcgull::node>& parent) :
        Action(device, interface_name, {
                {
                        {"GetProfile", {this, &ChangeProfile::getProfile, {"profile"}}},
                        {"SetProfile", {this, &ChangeProfile::setProfile, {"profile"}}}
                },
                {},
                {}
        }), _config(config) {
}

void ChangeProfile::press() {
}

void ChangeProfile::release() {
    std::shared_lock lock(_config_mutex);
    if (_config.profile.has_value())
        _device->setProfileDelayed(_config.profile.value());
}

uint8_t ChangeProfile::reprogFlags() const {
    return backend::hidpp20::ReprogControls::TemporaryDiverted;
}

std::string ChangeProfile::getProfile() {
    std::shared_lock lock(_config_mutex);
    if (_config.profile.has_value())
        return _config.profile.value();
    else
        return "";
}

void ChangeProfile::setProfile(std::string profile) {
    std::unique_lock lock(_config_mutex);

    if (profile.empty())
        _config.profile->clear();
    else
        _config.profile = std::move(profile);
}
