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
#ifndef LOGID_ACTION_CHANGEHOSTACTION_H
#define LOGID_ACTION_CHANGEHOSTACTION_H

#include <libconfig.h++>
#include "Action.h"
#include "../backend/hidpp20/features/ChangeHost.h"

namespace logid {
namespace actions
{
    class ChangeHostAction : public Action
    {
    public:
        ChangeHostAction(Device* device, libconfig::Setting& config);

        virtual void press();
        virtual void release();

        virtual uint8_t reprogFlags() const;

        class Config : public Action::Config
        {
        public:
            Config(Device* device, libconfig::Setting& setting);
            uint8_t nextHost(backend::hidpp20::ChangeHost::HostInfo info);
        private:
            bool _offset;
            int _host;
        };

    protected:
        std::shared_ptr<backend::hidpp20::ChangeHost> _change_host;
        Config _config;
    };
}}

#endif //LOGID_ACTION_CHANGEHOSTACTION_H
