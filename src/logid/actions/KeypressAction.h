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
        static const char* interface_name;

        KeypressAction(Device* dev,
                       config::KeypressAction& config,
                       const std::shared_ptr<ipcgull::node>& parent);

        virtual void press();
        virtual void release();

        [[nodiscard]] std::vector<std::string> getKeys() const;
        void setKeys(const std::vector<std::string>& keys);

        virtual uint8_t reprogFlags() const;
    protected:
        mutable std::mutex _config_lock;
        config::KeypressAction& _config;
        std::list<uint> _keys;

        void _setConfig();
    };
}}

#endif //LOGID_ACTION_KEYPRESS_H
