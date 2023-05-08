/*
 * Copyright 2019-2023 PixlOne
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

#include <features/DeviceFeature.h>
#include <backend/hidpp20/features/SmartShift.h>
#include <ipcgull/interface.h>
#include <config/schema.h>
#include <shared_mutex>

namespace logid::features {
    class SmartShift : public DeviceFeature {
    public:
        void configure() final;

        void listen() final;

        void setProfile(config::Profile& profile) final;

        typedef backend::hidpp20::SmartShift::Status Status;

        [[nodiscard]] Status getStatus() const;

        void setStatus(Status status);

        [[nodiscard]] const backend::hidpp20::SmartShift::Defaults& getDefaults() const;

        [[nodiscard]] bool supportsTorque() const;

    protected:
        explicit SmartShift(Device* dev);

    private:
        mutable std::shared_mutex _config_mutex;
        std::reference_wrapper<std::optional<config::SmartShift>> _config;
        std::shared_ptr<backend::hidpp20::SmartShift> _smartshift;

        backend::hidpp20::SmartShift::Defaults _defaults{};
        bool _torque_support = false;

        class IPC : public ipcgull::interface {
        public:
            explicit IPC(SmartShift* parent);

            [[nodiscard]] std::tuple<uint8_t, uint8_t, uint8_t> getConfig() const;

            void setActive(bool active, bool clear);

            void setThreshold(uint8_t threshold, bool clear);

            void setTorque(uint8_t torque, bool clear);

        private:
            SmartShift& _parent;
        };

        std::shared_ptr<IPC> _ipc_interface;
    };
}

#endif //LOGID_FEATURE_SMARTSHIFT_H
