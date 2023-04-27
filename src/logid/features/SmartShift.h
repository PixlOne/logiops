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
#ifndef LOGID_FEATURE_SMARTSHIFT_H
#define LOGID_FEATURE_SMARTSHIFT_H

#include <ipcgull/interface.h>
#include "../backend/hidpp20/features/SmartShift.h"
#include "DeviceFeature.h"
#include "../config/schema.h"

namespace logid::features {
    class SmartShift : public DeviceFeature {
    public:
        explicit SmartShift(Device* dev);

        void configure() final;

        void listen() final;

        typedef backend::hidpp20::SmartShift::SmartshiftStatus Status;

        [[nodiscard]] Status getStatus() const;

        void setStatus(Status status);

    private:
        std::optional<config::SmartShift>& _config;
        std::shared_ptr<backend::hidpp20::SmartShift> _smartshift;

        class IPC : public ipcgull::interface {
        public:
            explicit IPC(SmartShift* parent);

            [[nodiscard]] std::tuple<bool, uint8_t> getStatus() const;;

            void setActive(bool active);

            void setThreshold(uint8_t threshold);

            [[nodiscard]] std::tuple<bool, bool, bool, uint8_t> getDefault() const;

            void clearDefaultActive();

            void setDefaultActive(bool active);

            void clearDefaultThreshold();

            void setDefaultThreshold(uint8_t threshold);

        private:
            SmartShift& _parent;
        };

        std::shared_ptr<IPC> _ipc;
    };
}

#endif //LOGID_FEATURE_SMARTSHIFT_H
