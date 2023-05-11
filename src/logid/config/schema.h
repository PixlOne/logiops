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

#include <config/types.h>

namespace logid::actions {
    class ChangeDPI;

    class ChangeHostAction;

    class ChangeProfile;

    class CycleDPI;

    class GestureAction;

    class KeypressAction;

    class NullAction;

    class ToggleHiresScroll;

    class ToggleSmartShift;

    class AxisGesture;

    class IntervalGesture;

    class NullGesture;

    class ReleaseGesture;

    class ThresholdGesture;
}

namespace logid::config {
    struct keys {
        static const char name[];
        static const char cid[];
        static const char direction[];
    };

    struct NoAction : public signed_group<std::string> {
        typedef actions::NullAction action;

        NoAction() : signed_group<std::string>("type", "None") {}
    };

    struct KeypressAction : public signed_group<std::string> {
        typedef actions::KeypressAction action;
        std::optional<
                std::variant<std::string, uint,
                        std::list<std::variant<uint, std::string>>>> keys;

        KeypressAction() : signed_group<std::string>(
                "type", "Keypress",
                {"keys"}, &KeypressAction::keys) {
        }
    };

    struct ToggleSmartShift : public signed_group<std::string> {
        typedef actions::ToggleSmartShift action;

        ToggleSmartShift() :
                signed_group<std::string>("type", "ToggleSmartShift") {}
    };

    struct ToggleHiresScroll : public signed_group<std::string> {
        typedef actions::ToggleHiresScroll action;

        ToggleHiresScroll() :
                signed_group<std::string>("type", "ToggleHiresScroll") {}
    };

    struct CycleDPI : public signed_group<std::string> {
        typedef actions::CycleDPI action;
        std::optional<std::list<int>> dpis;
        std::optional<int> sensor;

        CycleDPI() : signed_group<std::string>(
                "type", "CycleDPI",
                {"dpis", "sensor"},
                &CycleDPI::dpis,
                &CycleDPI::sensor) {}
    };

    struct ChangeDPI : public signed_group<std::string> {
        typedef actions::ChangeDPI action;
        std::optional<int> inc;
        std::optional<int> sensor;

        ChangeDPI() : signed_group<std::string>(
                "type", "ChangeDPI",
                {"inc", "sensor"},
                &ChangeDPI::inc,
                &ChangeDPI::sensor) {}
    };

    struct ChangeHost : public signed_group<std::string> {
        typedef actions::ChangeHostAction action;
        std::optional<std::variant<int, std::string>> host;

        ChangeHost() : signed_group<std::string>(
                "type", "ChangeHost",
                {"host"}, &ChangeHost::host) {}
    };

    struct ChangeProfile : public signed_group<std::string> {
        typedef actions::ChangeProfile action;
        std::optional<std::string> profile;

        ChangeProfile() : signed_group<std::string>("type", "ChangeProfile",
                                                    {"profile"}, &ChangeProfile::profile) {}
    };

    typedef std::variant<
            NoAction,
            KeypressAction,
            ToggleSmartShift,
            ToggleHiresScroll,
            CycleDPI,
            ChangeDPI,
            ChangeHost,
            ChangeProfile
    > BasicAction;

    struct AxisGesture : public signed_group<std::string> {
        typedef actions::AxisGesture gesture;
        std::optional<int> threshold;
        std::optional<std::variant<std::string, uint>> axis;
        std::optional<double> axis_multiplier;

        AxisGesture() : signed_group("mode", "Axis",
                                     {"threshold", "axis", "axis_multiplier"},
                                     &AxisGesture::threshold,
                                     &AxisGesture::axis,
                                     &AxisGesture::axis_multiplier) {}
    };

    struct IntervalGesture : public signed_group<std::string> {
        typedef actions::IntervalGesture gesture;
        std::optional<int> threshold;
        std::optional<BasicAction> action;
        std::optional<int> interval;
    protected:
        explicit IntervalGesture(const std::string& name) : signed_group(
                "mode", name,
                {"threshold", "action", "interval"},
                &IntervalGesture::threshold,
                &IntervalGesture::action,
                &IntervalGesture::interval) {}

    public:
        IntervalGesture() : IntervalGesture("OnInterval") {}
    };

    struct FewPixelsGesture : public IntervalGesture {
        FewPixelsGesture() : IntervalGesture("OnFewPixels") {}
    };

    struct ReleaseGesture : public signed_group<std::string> {
        typedef actions::ReleaseGesture gesture;
        std::optional<int> threshold;
        std::optional<BasicAction> action;

        ReleaseGesture() : signed_group("mode", "OnRelease",
                                        {"threshold", "action"},
                                        &ReleaseGesture::threshold,
                                        &ReleaseGesture::action) {}
    };

    struct ThresholdGesture : public signed_group<std::string> {
        typedef actions::ThresholdGesture gesture;
        std::optional<int> threshold;
        std::optional<BasicAction> action;

        ThresholdGesture() : signed_group("mode", "OnThreshold",
                                          {"threshold", "action"},
                                          &ThresholdGesture::threshold,
                                          &ThresholdGesture::action) {}
    };

    struct NoGesture : public signed_group<std::string> {
        typedef actions::NullGesture gesture;
        std::optional<int> threshold;

        NoGesture() : signed_group("mode", "NoPress",
                                   {"threshold"},
                                   &NoGesture::threshold) {}
    };

    typedef std::variant<
            NoGesture,
            AxisGesture,
            IntervalGesture,
            FewPixelsGesture,
            ReleaseGesture,
            ThresholdGesture
    > Gesture;


    struct GestureAction : public signed_group<std::string> {
        typedef actions::GestureAction action;
        std::optional<map<std::string, Gesture, string_literal_of<keys::direction>,
                less_caseless<std::string>>> gestures;

        GestureAction() : signed_group<std::string>(
                "type", "Gestures",
                {"gestures"},
                &GestureAction::gestures) {}
    };

    typedef std::variant<
            NoAction,
            KeypressAction,
            ToggleSmartShift,
            ToggleHiresScroll,
            CycleDPI,
            ChangeDPI,
            ChangeHost,
            ChangeProfile,
            GestureAction
    > Action;

    struct Button : public group {
        std::optional<Action> action;

        Button() : group({"action"},
                         &Button::action) {}
    };

    struct SmartShift : public group {
        std::optional<bool> on;
        std::optional<unsigned int> threshold;
        std::optional<unsigned int> torque;

        SmartShift() : group({"on", "threshold", "torque"},
                             &SmartShift::on, &SmartShift::threshold, &SmartShift::torque) {}
    };


    struct HiresScroll : public group {
        std::optional<bool> hires;
        std::optional<bool> invert;
        std::optional<bool> target;
        std::optional<Gesture> up;
        std::optional<Gesture> down;

        HiresScroll() : group({"hires", "invert", "target", "up", "down"},
                              &HiresScroll::hires,
                              &HiresScroll::invert,
                              &HiresScroll::target,
                              &HiresScroll::up,
                              &HiresScroll::down) {}
    };

    typedef std::variant<int, std::list<int>> DPI;

    struct ThumbWheel : public group {
        std::optional<bool> divert;
        std::optional<bool> invert;
        std::optional<Gesture> left;
        std::optional<Gesture> right;
        std::optional<BasicAction> proxy;
        std::optional<BasicAction> touch;
        std::optional<BasicAction> tap;

        ThumbWheel() : group({"divert", "invert", "left", "right",
                              "proxy", "touch", "tap"},
                             &ThumbWheel::divert, &ThumbWheel::invert,
                             &ThumbWheel::left, &ThumbWheel::right,
                             &ThumbWheel::proxy, &ThumbWheel::touch,
                             &ThumbWheel::tap) {}
    };

    typedef map<uint16_t, Button, string_literal_of<keys::cid>> RemapButton;

    struct Profile : public group {
        std::optional<DPI> dpi;
        std::optional<SmartShift> smartshift;
        std::optional<std::variant<bool, HiresScroll>> hiresscroll;
        std::optional<ThumbWheel> thumbwheel;
        std::optional<RemapButton> buttons;

        Profile() : group({"dpi", "smartshift", "hiresscroll",
                           "buttons", "thumbwheel"},
                          &Profile::dpi, &Profile::smartshift,
                          &Profile::hiresscroll, &Profile::buttons,
                          &Profile::thumbwheel) {}
    };

    struct Device : public group {
        ipcgull::property<std::string> default_profile;
        map<std::string, Profile, string_literal_of<keys::name>> profiles;

        Device() : group({"default_profile", "profiles"},
                         &Device::default_profile,
                         &Device::profiles),
                   default_profile(ipcgull::property_full_permissions, "") {
        }
    };

    struct Config : public group {
        std::optional<map<std::string,
                std::variant<Device, Profile>, string_literal_of<keys::name>>> devices;
        std::optional<std::set<uint16_t>> ignore;
        std::optional<double> io_timeout;
        std::optional<int> workers;

        Config() : group({"devices", "ignore", "io_timeout", "workers"},
                         &Config::devices,
                         &Config::ignore,
                         &Config::io_timeout,
                         &Config::workers) {}
    };
}

#endif //LOGID_CONFIG_SCHEMA_H
