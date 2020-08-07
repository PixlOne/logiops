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
#include "SwitchProfileAction.h"
#include "../util/log.h"
#include "../backend/hidpp20/features/ReprogControls.h"

logid::actions::SwitchProfileAction::SwitchProfileAction(logid::Device *device, libconfig::Setting &config) : Action(device), _config(device, config) {

}

void logid::actions::SwitchProfileAction::press() {
    _pressed = true;
    auto switchType = _config.getSwitchType();
    if(switchType == Config::SwitchType_NEXT){

    }
    else if(switchType == Config::SwitchType_PREVIOUS){

    }
    else if(switchType == Config::SwitchType_DIRECT){

    }
}

void logid::actions::SwitchProfileAction::release() {
    _pressed = false;
}

uint8_t logid::actions::SwitchProfileAction::reprogFlags() const {
    return backend::hidpp20::ReprogControls::TemporaryDiverted;;
}

logid::actions::SwitchProfileAction::Config::Config(logid::Device *device, libconfig::Setting &config) : Action::Config(device){
    if(!config.isGroup()) {
        logPrintf(WARN, "Line %d: action must be an object, skipping.",
                  config.getSourceLine());
        return;
    }

    _switchType = SwitchType_UNKNOWN;
    try {
        auto &type = config.lookup("switchtype");
        if(type.getType() == libconfig::Setting::TypeString){
            std::string typestr = type;
            if(typestr == "next"){
                _switchType = SwitchType_NEXT;
            }
            else if(typestr == "prev"){
                _switchType = SwitchType_PREVIOUS;
            }
            else if(typestr == "direct"){
                try {
                    auto &profid = config.lookup("profile");
                    if(profid.isNumber()){
                        _directProfileID = profid;
                        _switchType = SwitchType_DIRECT;
                    }
                    else{
                        logPrintf(WARN, "Line %d: profile was not a number, skipping.",
                                  config.getSourceLine());
                        return;
                    }
                } catch (libconfig::SettingNotFoundException& e) {
                    logPrintf(WARN, "Line %d: profile is a required field with direct switchtype, skipping.",
                              config.getSourceLine());
                    return;
                }
            }
            else{
                logPrintf(WARN, "Line %d: %s is unknown profile switch type, skipping.",
                          config.getSourceLine(), typestr.c_str());
                return;
            }
        }
        else{
            logPrintf(WARN, "Line %d: switchtype is not string, skipping.",
                      config.getSourceLine());
            return;
        }
    } catch (libconfig::SettingNotFoundException& e) {
        logPrintf(WARN, "Line %d: switchtype is a required field, skipping.",
                  config.getSourceLine());
    }
}

logid::actions::SwitchProfileAction::Config::SwitchType
logid::actions::SwitchProfileAction::Config::getSwitchType() const {
    return _switchType;
}

int logid::actions::SwitchProfileAction::Config::getDirectProfileId() const {
    return _directProfileID;
}




