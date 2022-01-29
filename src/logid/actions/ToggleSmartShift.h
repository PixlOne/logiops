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
#ifndef LOGID_ACTION_TOGGLESMARTSHIFT_H
#define LOGID_ACTION_TOGGLESMARTSHIFT_H

#include <libconfig.h++>
#include "Action.h"
#include "../features/SmartShift.h"

namespace logid {
namespace actions {
    class ToggleSmartShift : public Action
    {
    public:
        static const char* interface_name;

        ToggleSmartShift(Device* dev,
                         const std::shared_ptr<ipcgull::node>& parent);
        ToggleSmartShift(Device* device,
                         [[maybe_unused]] config::ToggleSmartShift& action,
                         const std::shared_ptr<ipcgull::node>& parent) :
            ToggleSmartShift(device, parent) { }

        virtual void press();
        virtual void release();

        virtual uint8_t reprogFlags() const;
    protected:
        std::shared_ptr<features::SmartShift> _smartshift;
    private:
        class IPC : public ipcgull::interface
        {
        public:
            IPC();
        };

        std::shared_ptr<IPC> _ipc;
    };
}}

#endif //LOGID_ACTION_TOGGLESMARTSHIFT_H