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

RemapButton::RemapButton(Device *dev): DeviceFeature(dev), _config (dev)
{
    try {
        _reprog_controls = hidpp20::ReprogControls::autoVersion(
                &dev->hidpp20());
    } catch(hidpp20::UnsupportedFeature& e) {
        throw UnsupportedFeature();
    }

    _reprog_controls->initCidMap();

    if(global_loglevel <= DEBUG) {
        #define FLAG(x) control.second.flags & hidpp20::ReprogControls::x ? \
            "YES" : ""
        #define ADDITIONAL_FLAG(x) control.second.additionalFlags & \
            hidpp20::ReprogControls::x ? "YES" : ""

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
        #undef FLAG
    }
}

RemapButton::~RemapButton()
{
    _device->hidpp20().removeEventHandler(EVENTHANDLER_NAME);
}

void RemapButton::configure()
{
    ///TODO: DJ reporting trickery if cannot be remapped
    for(const auto& i : _config.buttons()) {
        hidpp20::ReprogControls::ControlInfo info{};
        try {
            info = _reprog_controls->getControlIdInfo(i.first);
        } catch(hidpp20::Error& e) {
            if(e.code() == hidpp20::Error::InvalidArgument) {
                logPrintf(WARN, "%s: CID 0x%02x does not exist.",
                        _device->name().c_str(), i.first);
                continue;
            }
            throw e;
        }

        if((i.second->reprogFlags() & hidpp20::ReprogControls::RawXYDiverted) &&
                (!_reprog_controls->supportsRawXY() || !(info.additionalFlags &
                hidpp20::ReprogControls::RawXY)))
            logPrintf(WARN, "%s: Cannot divert raw XY movements for CID "
                            "0x%02x", _device->name().c_str(), i.first);

        hidpp20::ReprogControls::ControlInfo report{};
        report.controlID = i.first;
        report.flags = HIDPP20_REPROG_REBIND;
        report.flags |= i.second->reprogFlags();
        _reprog_controls->setControlReporting(i.first, report);
    }
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
                for(const auto& button : this->_config.buttons())
                    if(button.second->pressed())
                        button.second->move(divertedXY.x, divertedXY.y);
            }
        };

        _device->hidpp20().addEventHandler(EVENTHANDLER_NAME, handler);
    }
}

bool RemapButton::onHiresScroll(int16_t deltaV)
{
    bool handled = false;
    for(const auto& button : this->_config.buttons())
        if(button.second->pressed()) {
            button.second->scroll(deltaV);
                handled = true;
        }
    return handled;
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
            auto action = _config.buttons().find(i);
            if(action != _config.buttons().end())
                action->second->press();
        }
    }

    // Release all removed buttons
    for(auto& i : _pressed_buttons) {
        auto action = _config.buttons().find(i);
        if(action != _config.buttons().end())
            action->second->release();
    }

    _pressed_buttons = new_state;
}

RemapButton::Config::Config(Device *dev) : DeviceFeature::Config(dev)
{
    try {
        auto& config_root = dev->config().getSetting("buttons");
        if(!config_root.isList()) {
            logPrintf(WARN, "Line %d: buttons must be a list.",
                    config_root.getSourceLine());
            return;
        }
        int button_count = config_root.getLength();
        for(int i = 0; i < button_count; i++)
            _parseButton(config_root[i]);
    } catch(libconfig::SettingNotFoundException& e) {
        // buttons not configured, use default
    }
}

void RemapButton::Config::_parseButton(libconfig::Setting &setting)
{
    if(!setting.isGroup()) {
        logPrintf(WARN, "Line %d: button must be an object, ignoring.",
                setting.getSourceLine());
        return;
    }

    uint16_t cid;
    try {
        auto& cid_setting = setting["cid"];
        if(!cid_setting.isNumber()) {
            logPrintf(WARN, "Line %d: cid must be a number, ignoring.",
                    cid_setting.getSourceLine());
            return;
        }
        cid = (int)cid_setting;
    } catch(libconfig::SettingNotFoundException& e) {
        logPrintf(WARN, "Line %d: cid is required, ignoring.",
                  setting.getSourceLine());
        return;
    }

    try {
        _buttons.emplace(cid, Action::makeAction(_device,
                setting["action"]));
    } catch(libconfig::SettingNotFoundException& e) {
        logPrintf(WARN, "Line %d: action is required, ignoring.",
                  setting.getSourceLine());
    } catch(InvalidAction& e) {
        logPrintf(WARN, "Line %d: %s is not a valid action, ignoring.",
                setting["action"].getSourceLine(), e.what());
    }
}

const std::map<uint8_t, std::shared_ptr<Action>>& RemapButton::Config::buttons()
{
    return _buttons;
}
