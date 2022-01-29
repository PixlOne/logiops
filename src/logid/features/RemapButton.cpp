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
#include <sstream>
#include "../Device.h"
#include "RemapButton.h"
#include "../backend/hidpp20/Error.h"

using namespace logid::features;
using namespace logid::backend;
using namespace logid::actions;

#define HIDPP20_REPROG_REBIND (hidpp20::ReprogControls::ChangeTemporaryDivert \
| hidpp20::ReprogControls::ChangeRawXYDivert)

#define EVENTHANDLER_NAME "REMAP_BUTTON"

RemapButton::RemapButton(Device *dev): DeviceFeature(dev),
    _config (dev->activeProfile().buttons),
    _ipc_node (dev->ipcNode()->make_child("buttons"))
{
    try {
        _reprog_controls = hidpp20::ReprogControls::autoVersion(
                &dev->hidpp20());
    } catch(hidpp20::UnsupportedFeature& e) {
        throw UnsupportedFeature();
    }

    _reprog_controls->initCidMap();

    if(!_config.has_value())
        _config = config::RemapButton();

    for(const auto& control : _reprog_controls->getControls()) {
        const auto i = _buttons.size();
        Button::ConfigFunction func = [this, info=control.second](
                std::shared_ptr<actions::Action> action) {
            hidpp20::ReprogControls::ControlInfo report{};
            report.controlID = info.controlID;
            report.flags = HIDPP20_REPROG_REBIND;

            if(action) {
                if(( action->reprogFlags() &
                     hidpp20::ReprogControls::RawXYDiverted ) &&
                   ( !_reprog_controls->supportsRawXY() ||
                     !(info.additionalFlags & hidpp20::ReprogControls::RawXY) ))
                    logPrintf(WARN,
                              "%s: Cannot divert raw XY movements for CID "
                              "0x%02x", _device->name().c_str(),
                              info.controlID);

                report.flags |= action->reprogFlags();
            }
            _reprog_controls->setControlReporting(info.controlID, report);
        };
        _buttons.emplace(std::piecewise_construct,
                         std::forward_as_tuple(control.second.controlID),
                         std::forward_as_tuple(control.second, i,
                                               _device, func,
                                               _ipc_node,
                                               _config.value()[control.first]));
    }

    if(global_loglevel <= DEBUG) {
        #define FLAG(x) (control.second.flags & hidpp20::ReprogControls::x ? \
            "YES" : "")
        #define ADDITIONAL_FLAG(x) (control.second.additionalFlags & \
            hidpp20::ReprogControls::x ? "YES" : "")

        // Print CIDs, originally by zv0n
        logPrintf(DEBUG,  "%s:%d remappable buttons:",
                dev->hidpp20().devicePath().c_str(),
                dev->hidpp20().deviceIndex());
        logPrintf(DEBUG, "CID  | reprog? | fn key? | mouse key? | "
                         "gesture support?");
        for(const auto & control : _reprog_controls->getControls())
                logPrintf(DEBUG, "0x%02x | %-7s | %-7s | %-10s | %s",
                        control.first, FLAG(TemporaryDivertable), FLAG(FKey),
                        FLAG(MouseButton), ADDITIONAL_FLAG(RawXY));
        #undef ADDITIONAL_FLAG
        #undef FLAG
    }
}

RemapButton::~RemapButton()
{
    _device->hidpp20().removeEventHandler(EVENTHANDLER_NAME);
}

void RemapButton::configure()
{
    for(const auto& button : _buttons)
        button.second.configure();
}

void RemapButton::listen()
{
    if(_device->hidpp20().eventHandlers().find(EVENTHANDLER_NAME) ==
       _device->hidpp20().eventHandlers().end()) {
        auto handler = std::make_shared<hidpp::EventHandler>();
        handler->condition = [index=_reprog_controls->featureIndex()]
                                      (hidpp::Report& report)->bool {
            return (report.feature() == index) && ((report.function() ==
                hidpp20::ReprogControls::DivertedButtonEvent) || (report
                .function() == hidpp20::ReprogControls::DivertedRawXYEvent));
        };

        handler->callback = [this](hidpp::Report& report)->void {
            if(report.function() ==
                hidpp20::ReprogControls::DivertedButtonEvent)
                this->_buttonEvent(_reprog_controls->divertedButtonEvent(
                        report));
            else { // RawXY
                auto divertedXY = _reprog_controls->divertedRawXYEvent(report);
                for(const auto& button : this->_buttons)
                    if(button.second.pressed())
                        button.second.move(divertedXY.x, divertedXY.y);
            }
        };

        _device->hidpp20().addEventHandler(EVENTHANDLER_NAME, handler);
    }
}

void RemapButton::_buttonEvent(const std::set<uint16_t>& new_state)
{
    // Ensure I/O doesn't occur while updating button state
    std::lock_guard<std::mutex> lock(_button_lock);

    // Press all added buttons
    for(const auto& i : new_state) {
        auto old_i = _pressed_buttons.find(i);
        if(old_i != _pressed_buttons.end()) {
            _pressed_buttons.erase(old_i);
        } else {
            auto action = _buttons.find(i);
            if(action != _buttons.end())
                action->second.press();
        }
    }

    // Release all removed buttons
    for(auto& i : _pressed_buttons) {
        auto action = _buttons.find(i);
        if(action != _buttons.end())
            action->second.release();
    }

    _pressed_buttons = new_state;
}

Button::Button(Info info, int index,
               Device *device, ConfigFunction conf_func,
               std::shared_ptr<ipcgull::node> root,
               config::Button &config) :
               _node (root->make_child(std::to_string(index))),
               _device (device), _conf_func (std::move(conf_func)),
               _config (config),
               _info (info)
{
    if(_config.action.has_value()) {
        try {
            _action = Action::makeAction(_device, _config.action.value(), _node);
        } catch(std::exception& e) {
            logPrintf(WARN, "Error creating button action: %s",
                      e.what());
        }
    }

    _interface = _node->make_interface<IPC>(this, _info);
}

void Button::press() const {
    if(_action)
        _action->press();
}

void Button::release() const {
    if(_action)
        _action->release();
}

void Button::move(int16_t x, int16_t y) const {
    if(_action)
        _action->move(x, y);
}

bool Button::pressed() const {
    if(_action)
        return _action->pressed();
    return false;
}

void Button::configure() const {
    _conf_func(_action);
}

Button::IPC::IPC(Button* parent,
                 const Info& info) :
        ipcgull::interface("pizza.pixl.LogiOps.Device.Button", {}, {
                {"ControlID", ipcgull::property<uint16_t>(
                        ipcgull::property_readable, info.controlID)},
                {"Remappable", ipcgull::property<const bool>(
                        ipcgull::property_readable,
                        info.flags & hidpp20::ReprogControls::TemporaryDivertable)},
                {"GestureSupport", ipcgull::property<const bool>(
                        ipcgull::property_readable,
                        (info.additionalFlags & hidpp20::ReprogControls::RawXY)
                        )}
            }, {})
{
}
