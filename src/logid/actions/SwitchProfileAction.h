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
#ifndef LOGID_ACTION_SWITCHPROFILE_H
#define LOGID_ACTION_SWITCHPROFILE_H

#include <vector>
#include <libconfig.h++>
#include "Action.h"

namespace logid {
    namespace actions
    {
        /**
         * @brief class that represents an action to switch device profile
         */
        class SwitchProfileAction : public Action
        {
        public:
            SwitchProfileAction(Device* device, libconfig::Setting& config);

            virtual void press();
            virtual void release();

            virtual uint8_t reprogFlags() const;

            class Config : public Action::Config
            {
            public:
                explicit Config(Device* device, libconfig::Setting& root);
                enum SwitchType{
                    SwitchType_UNKNOWN = 0,
                    SwitchType_NEXT,
                    SwitchType_PREVIOUS,
                    SwitchType_DIRECT
                };
                SwitchType getSwitchType() const;
                int getDirectProfileId() const;
            protected:
                /**
                 * @brief enum value describing what kind of profile switch it is
                 */
                SwitchType _switchType;
                /**
                 * @brief int describing what profile to switch to if switchType == SwitchType_DIRECT
                 */
                int _directProfileID = 0;
            };

        protected:
            Config _config;
        };
    }}


#endif //LOGIOPS_SWITCHPROFILEACTION_H
