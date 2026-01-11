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
#include <actions/GestureAction.h>
#include <backend/hidpp20/features/ReprogControls.h>
#include <util/log.h>
#include <algorithm>

using namespace logid::actions;
using namespace logid;
using namespace logid::backend;

const char* GestureAction::interface_name = "Gesture";

GestureAction::Direction GestureAction::toDirection(std::string direction) {
    std::transform(direction.begin(), direction.end(), direction.begin(),
                   ::tolower);
    if (direction == "up")
        return Up;
    else if (direction == "down")
        return Down;
    else if (direction == "left")
        return Left;
    else if (direction == "right")
        return Right;
    else if(direction == "scrollup")
        return ScrollUp;
    else if(direction == "scrolldown")
        return ScrollDown;
    else if (direction == "none")
        return None;
    else
        throw std::invalid_argument("direction");
}

std::string GestureAction::fromDirection(Direction direction) {
    switch (direction) {
        case Up:
            return "up";
        case Down:
            return "down";
        case Left:
            return "left";
        case Right:
            return "right";
        case ScrollUp:
            return "scrollup";
        case ScrollDown:
            return "scrolldown";
        case None:
            return "none";
    }

    // This shouldn't happen
    throw InvalidGesture();
}

GestureAction::Direction GestureAction::toDirection(int32_t x, int32_t y, int16_t s) {
    if(isScroll())
        return s > 0 ? ScrollUp : ScrollDown;
    else if(isVertical())
        return y < 0 ? Up : Down;
    else if(isHorizontal())
        return x < 0 ? Left : Right;
    else
        return None;
}

GestureAction::GestureAction(Device* dev, config::GestureAction& config,
                             const std::shared_ptr<ipcgull::node>& parent) :
        Action(dev, interface_name,
               {
                       {
                               {"SetGesture", {this, &GestureAction::setGesture, {"direction", "type"}}}
                       },
                       {},
                       {}
               }),
        _node(parent->make_child("gestures")), _config(config) {
    if (_config.gestures.has_value()) {
        auto& gestures = _config.gestures.value();
        for (auto&& x: gestures) {
            try {
                auto direction = toDirection(x.first);
                _gestures.emplace(
                        direction,Gesture::makeGesture(
                                dev, x.second,
                                _node->make_child(fromDirection(direction))));
                if (direction == None) {
                    auto& gesture = x.second;
                    std::visit([](auto&& x) {
                        x.threshold.emplace(0);
                    }, gesture);
                }
            } catch (std::invalid_argument& e) {
                logPrintf(WARN, "%s is not a direction", x.first.c_str());
            }
        }
    }
}

void GestureAction::press() {
    std::shared_lock lock(_config_mutex);

    _pressed = true;
    _x = 0, _y = 0;
    for (auto& gesture: _gestures)
        gesture.second->press(false);
}

void GestureAction::release() {
    std::shared_lock lock(_config_mutex);

    _pressed = false;
    Direction d = toDirection(_x, _y, _s);

    for (auto& gesture: _gestures) {
        gesture.second->release(gesture.first == d);
    }

    auto none_gesture = _gestures.find(None);
    if (d == None && none_gesture != _gestures.end()) {
        none_gesture->second->release(none_gesture->first == d);
    }
}


bool GestureAction::isScroll()
{
    auto scrollUp = _gestures.find(ScrollUp);
    bool isScrollUp = scrollUp == _gestures.end() ? false : scrollUp->second->metThreshold();
    auto scrollDown = _gestures.find(ScrollDown);
    bool isScrollDown = scrollDown == _gestures.end() ? false : scrollDown->second->metThreshold();
    return isScrollUp || isScrollDown;
}

bool GestureAction::isVertical()
{
    auto up = _gestures.find(Up);
    bool isUp = up == _gestures.end() ? false : up->second->metThreshold();
    auto down = _gestures.find(Down);
    bool isDown = down == _gestures.end() ? false : down->second->metThreshold();
    return isUp || isDown;
}

bool GestureAction::isHorizontal()
{
    auto left = _gestures.find(Left);
    bool isLeft = left == _gestures.end() ? false : left->second->metThreshold();
    auto right = _gestures.find(Right);
    bool isRight = right == _gestures.end() ? false : right->second->metThreshold();
    return isLeft || isRight;
}

void GestureAction::move3D(int16_t x, int16_t y, int16_t s) {
    std::shared_lock lock(_config_mutex);

    int32_t new_x = _x + x, new_y = _y + y, new_s = _s + s;

    if (abs(x) > 0) {
        if (_x < 0 && new_x >= 0) { // Left -> Origin/Right
            auto left = _gestures.find(Left);
            if (left != _gestures.end() && left->second)
                left->second->move((int16_t) _x);
            if (new_x) { // Ignore to origin
                auto right = _gestures.find(Right);
                if (right != _gestures.end() && right->second)
                    right->second->move((int16_t) new_x);
            }
        } else if (_x > 0 && new_x <= 0) { // Right -> Origin/Left
            auto right = _gestures.find(Right);
            if (right != _gestures.end() && right->second)
                right->second->move((int16_t) -_x);
            if (new_x) { // Ignore to origin
                auto left = _gestures.find(Left);
                if (left != _gestures.end() && left->second)
                    left->second->move((int16_t) -new_x);
            }
        } else if (new_x < 0) { // Origin/Left to Left
            auto left = _gestures.find(Left);
            if (left != _gestures.end() && left->second)
                left->second->move((int16_t) -x);
        } else if (new_x > 0) { // Origin/Right to Right
            auto right = _gestures.find(Right);
            if (right != _gestures.end() && right->second)
                right->second->move(x);
        }
    }

    if (abs(y) > 0) {
        if (_y > 0 && new_y <= 0) { // Up -> Origin/Down
            auto up = _gestures.find(Up);
            if (up != _gestures.end() && up->second)
                up->second->move((int16_t) _y);
            if (new_y) { // Ignore to origin
                auto down = _gestures.find(Down);
                if (down != _gestures.end() && down->second)
                    down->second->move((int16_t) new_y);
            }
        } else if (_y < 0 && new_y >= 0) { // Down -> Origin/Up
            auto down = _gestures.find(Down);
            if (down != _gestures.end() && down->second)
                down->second->move((int16_t) -_y);
            if (new_y) { // Ignore to origin
                auto up = _gestures.find(Up);
                if (up != _gestures.end() && up->second)
                    up->second->move((int16_t) -new_y);
            }
        } else if (new_y < 0) { // Origin/Up to Up
            auto up = _gestures.find(Up);
            if (up != _gestures.end() && up->second)
                up->second->move((int16_t) -y);
        } else if (new_y > 0) {// Origin/Down to Down
            auto down = _gestures.find(Down);
            if (down != _gestures.end() && down->second)
                down->second->move(y);
        }
    }

    if (abs(s) > 0) {
       if(_s > 0 && new_s <= 0) { // ScrollDown -> Origin/ScrollUp
            auto down = _gestures.find(ScrollDown);
           if(down != _gestures.end())
                down->second->move(_s);
            if(new_s) { // Ignore to origin
                auto up = _gestures.find(ScrollUp);
                if(up != _gestures.end())
                    up->second->move(new_s);
            }
        } else if(_s < 0 && new_s >= 0) { // ScrollUp -> Origin/ScrollDown
            auto up = _gestures.find(ScrollUp);
            if(up != _gestures.end())
                up->second->move(-_s);
            if(new_s) { // Ignore to origin
                auto down = _gestures.find(ScrollDown);
                if(down != _gestures.end())
                    down->second->move(-new_s);
            }
        } else if(new_s < 0) { // Origin/ScrollDown to ScrollDown
            auto down = _gestures.find(ScrollDown);
            if(down != _gestures.end())
                down->second->move(-s);
        } else if(new_s > 0) {// Origin/ScrollUp to ScrollUp
            auto up = _gestures.find(ScrollUp);
            if(up != _gestures.end())
                up->second->move(s);
        }
    }

    _x = new_x;
    _y = new_y;
    _s = new_s;
}

uint8_t GestureAction::reprogFlags() const {
    return (hidpp20::ReprogControls::TemporaryDiverted |
            hidpp20::ReprogControls::RawXYDiverted);
}

void GestureAction::setGesture(const std::string& direction, const std::string& type) {
    std::unique_lock lock(_config_mutex);

    Direction d = toDirection(direction);

    auto it = _gestures.find(d);

    if (it != _gestures.end()) {
        if (pressed()) {
            auto current = toDirection(_x, _y, _s);
            if (it->second)
                it->second->release(current == d);
        }
    }

    auto dir_name = fromDirection(d);

    auto& gesture = _config.gestures.value()[dir_name];

    _gestures[d].reset();
    try {
        _gestures[d] = Gesture::makeGesture(
                _device, type, gesture,
                _node->make_child(dir_name));
    } catch (InvalidGesture& e) {
        _gestures[d] = Gesture::makeGesture(
                _device, gesture,
                _node->make_child(dir_name));
        throw std::invalid_argument("Invalid gesture type");
    }

    if (d == None) {
        std::visit([](auto&& x) {
            x.threshold = 0;
        }, gesture);
    }
}
