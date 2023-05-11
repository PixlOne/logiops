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
#ifndef LOGID_BACKEND_HIDPP20_FEATURE_CHANGEHOST_H
#define LOGID_BACKEND_HIDPP20_FEATURE_CHANGEHOST_H

#include <backend/hidpp20/feature_defs.h>
#include <backend/hidpp20/Feature.h>

namespace logid::backend::hidpp20 {
    class ChangeHost : public Feature {
    public:
        static const uint16_t ID = FeatureID::CHANGE_HOST;

        [[nodiscard]] uint16_t getID() final { return ID; }

        explicit ChangeHost(Device* dev);

        enum Function {
            GetHostInfo = 0,
            SetCurrentHost = 1,
            GetCookies = 2,
            SetCookie = 3
        };

        struct HostInfo {
            uint8_t hostCount;
            uint8_t currentHost;
            bool enhancedHostSwitch;
        };

        HostInfo getHostInfo();

        void setHost(uint8_t host);

        [[maybe_unused]] [[maybe_unused]] std::vector<uint8_t> getCookies();

        [[maybe_unused]] [[maybe_unused]] void setCookie(uint8_t host, uint8_t cookie);

    private:
        uint8_t _host_count;
    };
}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_CHANGEHOST_H
