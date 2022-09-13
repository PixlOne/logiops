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
#ifndef LOGID_ACTION_KEYPRESS_H
#define LOGID_ACTION_KEYPRESS_H

#include <vector>
#include <libconfig.h++>
#include "Action.h"

namespace logid {
namespace actions {
    class KeypressAction : public Action
    {
    public:
        KeypressAction(Device* dev, libconfig::Setting& config);

        virtual void press();
        virtual void release();

        virtual uint8_t reprogFlags() const;

        class Config : public Action::Config
        {
        public:
            explicit Config(Device* device, libconfig::Setting& root);
            std::vector<unsigned int>& keys();
        protected:
            std::vector<unsigned int> _keys;
        };
    protected:
        Config _config;
    };
}}

#endif //LOGID_ACTION_KEYPRESS_H
