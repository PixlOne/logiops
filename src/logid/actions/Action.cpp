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

#include <algorithm>
#include "Action.h"
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

namespace logid::actions {
    template<typename T>
    struct action_type {
        typedef typename T::action type;
    };

    template<typename T>
    struct action_type<const T> : action_type<T> {
    };

    template<typename T>
    struct action_type<T&> : action_type<T> {
    };

    template<typename T>
    std::shared_ptr<Action> _makeAction(
            Device* device, T& action,
            const std::shared_ptr<ipcgull::node>& parent) {
        return parent->make_interface<typename action_type<T>::type>(
                device, std::forward<T&>(action), parent);
    }

    template<typename T>
    std::shared_ptr<Action> _makeAction(
            Device* device, const std::string& name,
            std::optional<T>& config,
            const std::shared_ptr<ipcgull::node>& parent) {
        if (name == ChangeDPI::interface_name) {
            config = config::ChangeDPI();
        } else if (name == CycleDPI::interface_name) {
            config = config::CycleDPI();
        } else if (name == KeypressAction::interface_name) {
            config = config::KeypressAction();
        } else if (name == NullAction::interface_name) {
            config = config::NoAction();
        } else if (name == ChangeHostAction::interface_name) {
            config = config::ChangeHost();
        } else if (name == ToggleHiresScroll::interface_name) {
            config = config::ToggleHiresScroll();
        } else if (name == ToggleSmartShift::interface_name) {
            config = config::ToggleSmartShift();
        } else if (name == "Default") {
            config.reset();
            return nullptr;
        } else {
            throw InvalidAction(name);
        }

        return Action::makeAction(device, config.value(), parent);
    }
}

std::shared_ptr<Action> Action::makeAction(
        Device* device, const std::string& name,
        std::optional<config::BasicAction>& config,
        const std::shared_ptr<ipcgull::node>& parent) {
    return _makeAction(device, name, config, parent);
}

std::shared_ptr<Action> Action::makeAction(
        Device* device, const std::string& name,
        std::optional<config::Action>& config,
        const std::shared_ptr<ipcgull::node>& parent) {
    try {
        return _makeAction(device, name, config, parent);
    } catch (actions::InvalidAction& e) {
        if (name == GestureAction::interface_name) {
            config = config::GestureAction();
            return makeAction(device, config.value(), parent);
        }
        throw;
    }
}

std::shared_ptr<Action> Action::makeAction(
        Device* device, config::BasicAction& action,
        const std::shared_ptr<ipcgull::node>& parent) {
    std::shared_ptr<Action> ret;
    std::visit([&device, &ret, &parent](auto&& x) {
        ret = _makeAction(device, x, parent);
    }, action);
    return ret;
}

std::shared_ptr<Action> Action::makeAction(
        Device* device, config::Action& action,
        const std::shared_ptr<ipcgull::node>& parent) {
    std::shared_ptr<Action> ret;
    std::visit([&device, &ret, &parent](auto&& x) {
        ret = _makeAction(device, x, parent);
    }, action);
    return ret;
}

Action::Action(Device* device, const std::string& name, tables t) :
        ipcgull::interface("pizza.pixl.LogiOps.Action." + name, std::move(t)),
        _device(device), _pressed(false) {
}
