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
#include "ToggleSmartShift.h"
#include "ToggleHiresScroll.h"
#include "GestureAction.h"
#include "NullAction.h"
#include "CycleDPI.h"
#include "ChangeDPI.h"
#include "ChangeHostAction.h"

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
        auto& action_type = setting["type"];

        if(action_type.getType() != libconfig::Setting::TypeString) {
            logPrintf(WARN, "Line %d: Action type must be a string",
                    action_type.getSourceLine());
            throw InvalidAction();
        }

        std::string type = action_type;
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);

        if(type == "keypress")
            return std::make_shared<KeypressAction>(device, setting);
        else if(type == "togglesmartshift")
            return std::make_shared<ToggleSmartShift>(device);
        else if(type == "togglehiresscroll")
            return std::make_shared<ToggleHiresScroll>(device);
        else if(type == "gestures")
            return std::make_shared<GestureAction>(device, setting);
        else if(type == "cycledpi")
            return std::make_shared<CycleDPI>(device, setting);
        else if(type == "changedpi")
            return std::make_shared<ChangeDPI>(device, setting);
        else if(type == "none")
            return std::make_shared<NullAction>(device);
        else if(type == "changehost")
            return std::make_shared<ChangeHostAction>(device, setting);
        else
            throw InvalidAction(type);

    } catch(libconfig::SettingNotFoundException& e) {
        logPrintf(WARN, "Line %d: Action type is missing, ignoring.",
                setting.getSourceLine());
        throw InvalidAction();
    }
}
