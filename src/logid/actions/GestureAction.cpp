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
#include "GestureAction.h"
#include "../Device.h"
#include "../backend/hidpp20/features/ReprogControls.h"

using namespace logid::actions;
using namespace logid;
using namespace logid::backend;

GestureAction::Direction GestureAction::toDirection(std::string direction)
{
    std::transform(direction.begin(), direction.end(), direction.begin(),
            ::tolower);
    if(direction == "up")
        return Up;
    else if(direction == "down")
        return Down;
    else if(direction == "left")
        return Left;
    else if(direction == "right")
        return Right;
    else if(direction == "scrollup")
        return ScrollUp;
    else if(direction == "scrolldown")
        return ScrollDown;
    else if(direction == "none")
        return None;
    else
        throw std::invalid_argument("direction");
}

GestureAction::Direction GestureAction::toDirection(int16_t x, int16_t y, int16_t s)
{
    if(isScroll())
        return s > 0 ? ScrollUp : ScrollDown;
    else if(isVertical())
        return y < 0 ? Up : Down;
    else if(isHorizontal())
        return x < 0 ? Left : Right;
    else
        return None;
}

GestureAction::GestureAction(Device* dev, libconfig::Setting& config) :
    Action (dev), _config (dev, config)
{
}

void GestureAction::press()
{
    _pressed = true;
    _x = 0, _y = 0;
    for(auto& gesture : _config.gestures())
        gesture.second->press();
}

void GestureAction::release()
{
    _pressed = false;

    Direction d = toDirection(_x, _y, _s);

    for(auto& gesture : _config.gestures()) {
        gesture.second->release(gesture.first == d);
    }

    if(d == None && _config.noneAction()) {
        _config.noneAction()->press();
        _config.noneAction()->release();
    }
}

bool GestureAction::isScroll()
{
    auto scrollUp = _config.gestures().find(ScrollUp);
    bool isScrollUp = scrollUp == _config.gestures().end() ? false : scrollUp->second->metThreshold();
    auto scrollDown = _config.gestures().find(ScrollDown);
    bool isScrollDown = scrollDown == _config.gestures().end() ? false : scrollDown->second->metThreshold();
    return isScrollUp || isScrollDown;
}

bool GestureAction::isVertical()
{
    auto up = _config.gestures().find(Up);
    bool isUp = up == _config.gestures().end() ? false : up->second->metThreshold();
    auto down = _config.gestures().find(Down);
    bool isDown = down == _config.gestures().end() ? false : down->second->metThreshold();
    return isUp || isDown;
}

bool GestureAction::isHorizontal()
{
    auto left = _config.gestures().find(Left);
    bool isLeft = left == _config.gestures().end() ? false : left->second->metThreshold();
    auto right = _config.gestures().find(Right);
    bool isRight = right == _config.gestures().end() ? false : right->second->metThreshold();
    return isLeft || isRight;
}

void GestureAction::move3D(int16_t x, int16_t y, int16_t s)
{
    auto gesture = _config.gestures().end();
    int16_t axis = 0;

    bool isS = isScroll();
    bool isV = isVertical();
    bool isH = isHorizontal();

    if (!isS && !isV && x < 0) {
        gesture = _config.gestures().find(Left);
        axis = -x;
    }

    if (!isS && !isV && x > 0) {
        gesture = _config.gestures().find(Right);
        axis = x;
    }

    if (!isS && !isH && y < 0) {
        gesture = _config.gestures().find(Up);
        axis = -y;
    }

    if (!isS && !isH && y > 0) {
        gesture = _config.gestures().find(Down);
        axis = y;
    }

    if (!isV && !isH && s > 0) {
        gesture = _config.gestures().find(ScrollUp);
        axis = s;
    }

    if (!isV && !isH && s < 0) {
        gesture = _config.gestures().find(ScrollDown);
        axis = -s;
    }

    if (gesture != _config.gestures().end())
        gesture->second->move(axis);

    _x += x; _y += y; _s += s;
}

uint8_t GestureAction::reprogFlags() const
{
    return (hidpp20::ReprogControls::TemporaryDiverted |
        hidpp20::ReprogControls::RawXYDiverted);
}

GestureAction::Config::Config(Device* device, libconfig::Setting &root) :
    Action::Config(device)
{
    try {
        auto& gestures = root["gestures"];

        if(!gestures.isList()) {
            logPrintf(WARN, "Line %d: gestures must be a list, ignoring.",
                    gestures.getSourceLine());
            return;
        }

        int gesture_count = gestures.getLength();

        for(int i = 0; i < gesture_count; i++) {
            if(!gestures[i].isGroup()) {
                logPrintf(WARN, "Line %d: gesture must be a group, skipping.",
                        gestures[i].getSourceLine());
                continue;
            }

            Direction d;
            try {
                auto& direction = gestures[i]["direction"];
                if(direction.getType() != libconfig::Setting::TypeString) {
                    logPrintf(WARN, "Line %d: direction must be a string, "
                                    "skipping.", direction.getSourceLine());
                    continue;
                }

                try {
                    d = toDirection(direction);
                } catch(std::invalid_argument& e) {
                    logPrintf(WARN, "Line %d: Invalid direction %s",
                            direction.getSourceLine(), (const char*)direction);
                    continue;
                }
            } catch(libconfig::SettingNotFoundException& e) {
                logPrintf(WARN, "Line %d: direction is a required field, "
                                "skipping.", gestures[i].getSourceLine());
                continue;
            }

            if(_gestures.find(d) != _gestures.end() || (d == None && _none_action)) {
                logPrintf(WARN, "Line %d: Gesture is already defined for "
                                "this direction, duplicate ignored.",
                          gestures[i].getSourceLine());
                continue;
            }

            if(d == None) {
                try {
                    auto& mode = gestures[i]["mode"];
                    if(mode.getType() == libconfig::Setting::TypeString) {
                        std::string mode_str = mode;
                        std::transform(mode_str.begin(), mode_str.end(),
                                       mode_str.begin(), ::tolower);
                        if(mode_str == "nopress") // No action
                            continue;
                        else if(mode_str != "onrelease")
                            logPrintf(WARN, "Line %d: Only NoPress and "
                                            "OnRelease are supported for the "
                                            "None direction, defaulting to "
                                            "OnRelease.", mode.getSourceLine());
                    } else {
                        logPrintf(WARN, "Line %d: mode must be a string, "
                                        "defaulting to OnRelease",
                                        mode.getSourceLine());
                    }
                } catch(libconfig::SettingNotFoundException& e) {
                    // Default to OnRelease
                }

                try {
                    _none_action = Action::makeAction(_device,
                            gestures[i]["action"]);
                } catch (InvalidAction& e) {
                    logPrintf(WARN, "Line %d: %s is not a valid action, "
                                    "skipping.", gestures[i]["action"]
                                    .getSourceLine(), e.what());
                } catch (libconfig::SettingNotFoundException& e) {
                    logPrintf(WARN, "Line %d: action is a required field, "
                                    "skipping.", gestures[i].getSourceLine(),
                                    e.what());
                }
                continue;
            }

            try {
                _gestures.emplace(d, Gesture::makeGesture(_device,
                        gestures[i]));
            } catch(InvalidGesture& e) {
                logPrintf(WARN, "Line %d: Invalid gesture: %s",
                        gestures[i].getSourceLine(), e.what());
            }
        }

    } catch(libconfig::SettingNotFoundException& e) {
        logPrintf(WARN, "Line %d: gestures is a required field, ignoring.",
                root.getSourceLine());
    }
}

std::map<GestureAction::Direction, std::shared_ptr<Gesture>>&
    GestureAction::Config::gestures()
{
    return _gestures;
}

std::shared_ptr<Action> GestureAction::Config::noneAction()
{
    return _none_action;
}
