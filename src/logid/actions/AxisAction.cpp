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
#include "AxisAction.h"
#include "../util/log.h"
#include "../InputDevice.h"
#include "../backend/hidpp20/features/ReprogControls.h"
#include "../util/task.h"
#include "../util/workqueue.h"

using namespace logid::actions;
using namespace logid::backend;

AxisAction::AxisAction(Device *device, libconfig::Setting& config) :
    Action(device), _config (device, config)
{
    _config.repeatMutex().lock();
    std::shared_ptr<task> repeatTask = std::make_shared<task>([this]() {
        int axis = _config.axis();
        int move = _config.move();
        uint rate = _config.rate();
        logPrintf(DEBUG, "Started repeat task for axis %d with move %d at rate %d",
                    axis, move, rate);
        int low_res_axis = InputDevice::getLowResAxis(axis);
        int lowres_movement = move / _config.hiResMoveMultiplier();
        int hires_remainder = move % _config.hiResMoveMultiplier();
        while (true) {
            _config.repeatMutex().lock();
            if(low_res_axis != -1) {
//                 logPrintf(DEBUG, "Moving axis %d to %d:%d", axis, lowres_movement, hires_remainder);
                virtual_input->moveAxis(low_res_axis, lowres_movement);
                virtual_input->moveAxis(axis, hires_remainder);
            } else {
//                 logPrintf(DEBUG, "Moving axis %d to %d", axis, move);
                virtual_input->moveAxis(axis, move);
            }
            _config.repeatMutex().unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(rate));
        }
    }, [this](std::exception& e){
        logPrintf(ERROR, "Error during repeat: %s",
                    e.what());
    });
    global_workqueue->queue(repeatTask);
    repeatTask->waitStart();
}

void AxisAction::press()
{
    _pressed = true;
    _config.repeatMutex().unlock();

}

void AxisAction::release()
{
    _pressed = false;
    _config.repeatMutex().lock();
}

uint8_t AxisAction::reprogFlags() const
{
    return hidpp20::ReprogControls::TemporaryDiverted;
}

AxisAction::Config::Config(Device* device, libconfig::Setting& config) :
    Action::Config(device)
{
    if(!config.isGroup()) {
        logPrintf(WARN, "Line %d: action must be an object, skipping.",
                config.getSourceLine());
        return;
    }

    try {
        auto& axis = config["axis"];
        if(axis.isNumber()) {
            _axis = axis;
            virtual_input->registerAxis(axis);
            registerLowResAxis(axis);
            if (registerLowResAxis(axis)) {
                _move *= _hiResMoveMultiplier;
            }
        } else if(axis.getType() == libconfig::Setting::TypeString) {
            try {
                _axis = virtual_input->toAxisCode(axis);
                virtual_input->registerAxis(_axis);
                if (registerLowResAxis(_axis)) {
                    _move *= _hiResMoveMultiplier;
                }
            } catch(InputDevice::InvalidEventCode& e) {
                logPrintf(WARN, "Line %d: Invalid axis %s, skipping.",
                    axis.getSourceLine(), axis.c_str());
                return;
            }
        } else {
            logPrintf(WARN, "Line %d: axis must be string or int",
                    axis.getSourceLine(), axis.c_str());
            return;
        }
    } catch (libconfig::SettingNotFoundException& e) {
        logPrintf(WARN, "Line %d: axis is a required field, skipping.",
                config.getSourceLine());
    }

    try {
        auto& move = config["move"];
        if(move.isNumber()) {
            _move = move;
        } else {
            logPrintf(WARN, "Line %d: move must be int",
                    move.getSourceLine(), move.c_str());
            throw InvalidAction();
        }
    } catch (libconfig::SettingNotFoundException& e) {
        logPrintf(WARN, "Line %d: move is not defined, using default value %d.",
                config.getSourceLine(), _move);
    }
    if (_move == 0) {
        logPrintf(WARN, "Line %d: move == 0 - this is pointless, but continue with it.",
                config.getSourceLine());
    }


    try {
        auto& rate = config["rate"];
        if(rate.isNumber()) {
            if ((int)rate > 0) {
                _rate = rate;
            } else {
                logPrintf(WARN, "Line %d: rate is invalid, using default value %ums.",
                        config.getSourceLine(), _rate);
            }
        } else {
            logPrintf(WARN, "Line %d: rate must be int",
                    rate.getSourceLine(), rate.c_str());
            throw InvalidAction();
        }
    } catch (libconfig::SettingNotFoundException& e) {
        logPrintf(WARN, "Line %d: rate is not defined, using default value %ums.",
                config.getSourceLine(), _rate);
    }
    logPrintf(DEBUG, "Axis: configured axis %d with move %d at rate %d",
                    _axis, _move, _rate);
}

bool AxisAction::Config::registerLowResAxis(const uint axis_code) {
    bool registered = false;
    int low_res_axis = InputDevice::getLowResAxis(axis_code);
    if (low_res_axis != -1) {
        virtual_input->registerAxis(low_res_axis);
        registered = true;
    }
    return registered;
}

uint AxisAction::Config::axis()
{
    return _axis;
}

int AxisAction::Config::move()
{
    return _move;
}

uint AxisAction::Config::rate()
{
    return _rate;
}

int AxisAction::Config::hiResMoveMultiplier()
{
    return _hiResMoveMultiplier;
}

std::mutex& AxisAction::Config::repeatMutex()
{
    return _repeatMutex;
}
