/*
 * Copyright 2022 PixlOne
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
#ifndef LOGID_CONFIG_SCHEMA_H
#define LOGID_CONFIG_SCHEMA_H

#include "types.h"

namespace logid::config {
    struct NoAction : public signed_group<std::string> {
        NoAction() : signed_group<std::string>("type", "None") { }
    };

    struct KeypressAction : public signed_group<std::string> {
        std::variant<std::string, std::vector<std::string>> keys;
        KeypressAction() : signed_group<std::string>(
                "type", "Keypress",
                {"keys"}, &KeypressAction::keys) { }
    };

    struct ToggleSmartShift : public signed_group<std::string> {
        ToggleSmartShift() :
        signed_group<std::string>("type", "ToggleSmartShift") { }
    };

    struct ToggleHiresScroll : public signed_group<std::string> {
        ToggleHiresScroll() :
        signed_group<std::string>("type", "ToggleHiresScroll") { }
    };

    struct CycleDPI : public signed_group<std::string> {
        std::vector<int> dpis;
        std::optional<int> sensor;
        CycleDPI() : signed_group<std::string>(
                "type", "CycleDPI",
                {"dpis", "sensor"},
                &CycleDPI::dpis,
                &CycleDPI::sensor) { }
    };

    struct ChangeDPI : public signed_group<std::string> {
        int inc;
        std::optional<int> sensor;
        ChangeDPI() : signed_group<std::string>(
                "type", "ChangeDPI",
                {"inc", "sensor"},
                &ChangeDPI::inc,
                &ChangeDPI::sensor) { }
    };

    struct ChangeHost : public signed_group<std::string> {
        std::variant<int, std::string> host;
        ChangeHost() : signed_group<std::string>(
                "type", "ChangeHost",
                {"host"}, &ChangeHost::host) { }
    };

    struct Gesture;

    struct GestureAction : public signed_group<std::string> {
        std::optional<map<std::string, Gesture, "direction">> _gestures;

        GestureAction() : signed_group<std::string>(
                "type", "Gestures") { }
    };

    typedef std::variant<
            NoAction,
            KeypressAction,
            ToggleSmartShift,
            ToggleHiresScroll,
            CycleDPI,
            ChangeDPI,
            ChangeHost
    > BasicAction;

    typedef std::variant<
        NoAction,
        KeypressAction,
        ToggleSmartShift,
        ToggleHiresScroll,
        CycleDPI,
        ChangeDPI,
        ChangeHost,
        GestureAction
    > Action;

    struct Gesture : public group {
        std::optional<int> threshold;
        std::optional<std::string> mode;
        std::optional<std::string> axis;
        std::optional<double> axis_multiplier;
        Action action;

        Gesture() : group(
                {"threshold", "mode", "axis", "axis_multiplier"},
                &Gesture::threshold,
                &Gesture::mode,
                &Gesture::axis,
                &Gesture::axis_multiplier
        ) { }
    };

    struct Button : public group {
        std::optional<BasicAction> action;
        Button() : group({"action"},
                         &Button::action) { }
    };

    struct Smartshift : public group {
        std::optional<bool> on;
        std::optional<unsigned int> threshold;
        Smartshift() : group({"on", "threshold"},
                &Smartshift::on, &Smartshift::threshold) { }
    };


    struct Hiresscroll : public group {
        std::optional<bool> hires;
        std::optional<bool> invert;
        std::optional<bool> target;
        Hiresscroll() : group({"hires", "invert", "target"},
                &Hiresscroll::hires,
                &Hiresscroll::invert,
                &Hiresscroll::target) { }
    };

    using DPI = std::variant<int, std::vector<int>>;


    struct Profile : public group {
        std::optional<DPI> dpi;
        std::optional<Smartshift> smartshift;
        std::optional<std::variant<bool, Hiresscroll>> hiresscroll;
        std::optional<map<uint16_t, Button, "cid">> buttons;

        Profile() : group({"dpi", "smartshift", "hiresscroll", "buttons"},
                          &Profile::dpi,
                          &Profile::smartshift,
                          &Profile::hiresscroll,
                          &Profile::buttons) { }
    };

    struct Device : public group {
        std::string default_profile;
        map<std::string, Profile, "name"> profiles;

        Device() : group({"default_profile", "profiles"},
                &Device::default_profile,
                &Device::profiles) { }
    };

    struct Config : public group {
        std::optional<map<std::string,
        std::variant<Device, Profile>, "name">> devices;
        std::optional<std::vector<std::string>> ignore;
        std::optional<double> io_timeout;
        std::optional<int> workers;
        Config() : group({"devices", "ignore", "io_timeout", "workers"},
                         &Config::devices,
                         &Config::ignore,
                         &Config::io_timeout,
                         &Config::workers) { }
    };
}

#endif //LOGID_CONFIG_SCHEMA_H
