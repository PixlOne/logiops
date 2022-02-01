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
#include "Gesture.h"
#include "../../util/log.h"
#include "ReleaseGesture.h"
#include "ThresholdGesture.h"
#include "../../backend/hidpp20/features/ReprogControls.h"
#include "IntervalGesture.h"
#include "AxisGesture.h"
#include "NullGesture.h"

using namespace logid::actions;

Gesture::Gesture(Device *device) : _device (device)
{
}

Gesture::Config::Config(Device* device, libconfig::Setting& root,
        bool action_required) : _device (device)
{
    if(action_required) {
        try {
            _action = Action::makeAction(_device,
                                         root["action"]);
        } catch (libconfig::SettingNotFoundException &e) {
            throw InvalidGesture("action is missing");
        }

        if(_action->reprogFlags() & backend::hidpp20::ReprogControls::RawXYDiverted)
            throw InvalidGesture("gesture cannot require RawXY");
    }

    _threshold = LOGID_GESTURE_DEFAULT_THRESHOLD;
    try {
        auto& threshold = root["threshold"];
        if(threshold.getType() == libconfig::Setting::TypeInt) {
            _threshold = (int)threshold;
            if(_threshold <= 0) {
                _threshold = LOGID_GESTURE_DEFAULT_THRESHOLD;
                logPrintf(WARN, "Line %d: threshold must be positive, setting "
                                "to default (%d)", threshold.getSourceLine(),
                                _threshold);
            }
        } else
            logPrintf(WARN, "Line %d: threshold must be an integer, setting "
                            "to default (%d).", threshold.getSourceLine());
    } catch(libconfig::SettingNotFoundException& e) {
        // Ignore
    }
}

std::shared_ptr<Gesture> Gesture::makeGesture(Device *device,
        libconfig::Setting &setting)
{
    if(!setting.isGroup()) {
        logPrintf(WARN, "Line %d: Gesture is not a group, ignoring.",
                  setting.getSourceLine());
        throw InvalidGesture();
    }

    try {
        auto& gesture_mode = setting["mode"];

        if(gesture_mode.getType() != libconfig::Setting::TypeString) {
            logPrintf(WARN, "Line %d: Gesture mode must be a string,"
                            "defaulting to OnRelease.",
                      gesture_mode.getSourceLine());
            return std::make_shared<ReleaseGesture>(device, setting);
        }

        std::string type = gesture_mode;
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);

        if(type == "onrelease")
            return std::make_shared<ReleaseGesture>(device, setting);
        else if(type == "onthreshold")
            return std::make_shared<ThresholdGesture>(device, setting);
        else if(type == "oninterval" || type == "onfewpixels")
            return std::make_shared<IntervalGesture>(device, setting);
        else if(type == "axis")
            return std::make_shared<AxisGesture>(device, setting);
        else if(type == "nopress")
            return std::make_shared<NullGesture>(device, setting);
        else {
            logPrintf(WARN, "Line %d: Unknown gesture mode %s, defaulting to "
                            "OnRelease.", gesture_mode.getSourceLine(),
                      (const char*)gesture_mode);
            return std::make_shared<ReleaseGesture>(device, setting);
        }

    } catch(libconfig::SettingNotFoundException& e) {
        return std::make_shared<ReleaseGesture>(device, setting);
    }
}

int16_t Gesture::Config::threshold() const
{
    return _threshold;
}

std::shared_ptr<Action> Gesture::Config::action()
{
    return _action;
}
