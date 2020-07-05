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

#include <algorithm>
#include "Action.h"
#include "../util/log.h"
#include "KeypressAction.h"

using namespace logid;
using namespace logid::actions;

std::shared_ptr<Action> Action::makeAction(Device *device, libconfig::Setting
    &setting)
{
    if(!setting.isGroup()) {
        logPrintf(WARN, "Line %d: Action is not a group, ignoring.",
                setting.getSourceLine());
        throw InvalidAction();
    }

    try {
        auto& action_type = setting.lookup("type");

        if(action_type.getType() != libconfig::Setting::TypeString) {
            logPrintf(WARN, "Line %d: Action type must be a string",
                    action_type.getSourceLine());
            throw InvalidAction();
        }

        std::string type = action_type;
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);

        if(type == "keypress")
            return std::make_shared<KeypressAction>(device, setting);
        else
            throw InvalidAction(type);

    } catch(libconfig::SettingNotFoundException& e) {
        logPrintf(WARN, "Line %d: Action type is missing, ignoring.",
                setting.getSourceLine());
        throw InvalidAction();
    }
}