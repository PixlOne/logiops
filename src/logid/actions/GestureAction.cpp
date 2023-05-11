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
        case None:
            return "none";
    }

    // This shouldn't happen
    throw InvalidGesture();
}

GestureAction::Direction GestureAction::toDirection(int32_t x, int32_t y) {
    if (x >= 0 && y >= 0)
        return x >= y ? Right : Down;
    else if (x < 0 && y >= 0)
        return -x <= y ? Down : Left;
    else if (x <= 0 /* && y < 0 */)
        return x <= y ? Left : Up;
    else
        return x <= -y ? Up : Right;
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
    bool threshold_met = false;

    auto d = toDirection(_x, _y);
    auto primary_gesture = _gestures.find(d);
    if (primary_gesture != _gestures.end()) {
        threshold_met = primary_gesture->second->metThreshold();
        primary_gesture->second->release(true);
    }

    for (auto& gesture: _gestures) {
        if (gesture.first == d || gesture.first == None)
            continue;
        if (!threshold_met) {
            if (gesture.second->metThreshold()) {
                // If the primary gesture did not meet its threshold, use the
                // secondary one.
                threshold_met = true;
                gesture.second->release(true);
            }
        } else {
            gesture.second->release(false);
        }
    }

    auto none_gesture = _gestures.find(None);
    if (none_gesture != _gestures.end()) {
        none_gesture->second->release(!threshold_met);
    }
}

void GestureAction::move(int16_t x, int16_t y) {
    std::shared_lock lock(_config_mutex);

    int32_t new_x = _x + x, new_y = _y + y;

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

    _x = new_x;
    _y = new_y;
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
            auto current = toDirection(_x, _y);
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
