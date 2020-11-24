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
#ifndef LOGID_FEATURE_REMAPBUTTON_H
#define LOGID_FEATURE_REMAPBUTTON_H

#include "../backend/hidpp20/features/ReprogControls.h"
#include "DeviceFeature.h"
#include "../actions/Action.h"

namespace logid {
namespace features
{
    class RemapButton : public DeviceFeature
    {
    public:
        explicit RemapButton(Device* dev);
        ~RemapButton();
        virtual void configure();
        virtual void listen();
        virtual void onScroll(int16_t);

        class Config : public DeviceFeature::Config
        {
        public:
            explicit Config(Device* dev);
            const std::map<uint8_t, std::shared_ptr<actions::Action>>&
                buttons();
        protected:
            void _parseButton(libconfig::Setting& setting);
            std::map<uint8_t, std::shared_ptr<actions::Action>> _buttons;
        };
    private:
        void _buttonEvent(const std::set<uint16_t>& new_state);
        Config _config;
        std::shared_ptr<backend::hidpp20::ReprogControls> _reprog_controls;
        std::set<uint16_t> _pressed_buttons;
        std::mutex _button_lock;
    };
}}

#endif //LOGID_FEATURE_REMAPBUTTON_H
