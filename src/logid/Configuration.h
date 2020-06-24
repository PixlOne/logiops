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

#ifndef LOGID_CONFIGURATION_H
#define LOGID_CONFIGURATION_H

#include <map>
#include <libconfig.h++>
#include <hidpp20/ISmartShift.h>
#include "Actions.h"

namespace logid
{
    class DeviceConfig;
    class ButtonAction;
    enum class Action;

    class DeviceConfig
    {
    public:
        DeviceConfig();
        ~DeviceConfig();
        DeviceConfig(DeviceConfig* dc, Device* dev);
        DeviceConfig(const libconfig::Setting& root);
        const int* dpi = nullptr;
        HIDPP20::ISmartShift::SmartshiftStatus* smartshift = nullptr;
        const uint8_t* hiresscroll = nullptr;
        std::map<uint16_t, ButtonAction*> actions;
        const bool baseConfig = true;
    };

    class Configuration
    {
    public:
        Configuration(const char* config_file);
        Configuration() {}
        std::map<std::string, DeviceConfig*> devices;
        std::vector<uint16_t> blacklist;
    private:
        libconfig::Config cfg;
    };

    ButtonAction* parse_action(Action action, const libconfig::Setting* action_config, bool is_gesture=false);

    extern Configuration* global_config;
}

#endif //LOGID_CONFIGURATION_H