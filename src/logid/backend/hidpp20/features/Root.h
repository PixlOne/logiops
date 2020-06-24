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

#ifndef LOGID_BACKEND_HIDPP20_FEATURE_ROOT_H
#define LOGID_BACKEND_HIDPP20_FEATURE_ROOT_H

#include "../Feature.h"
#include "../EssentialFeature.h"
#include "../feature_defs.h"

namespace logid {
namespace backend {
namespace hidpp20
{
    class Root : public Feature
    {
    public:
        static const uint16_t ID = FeatureID::ROOT;
        virtual uint16_t getID() { return ID; }

        enum Function : uint8_t
        {
            GetFeature = 0,
            Ping = 1
        };

        explicit Root(Device* device);

        feature_info getFeature (uint16_t feature_id);
        std::tuple<uint8_t, uint8_t> getVersion();

        enum FeatureFlag : uint8_t
        {
            Obsolete = 1<<7,
            Hidden = 1<<6,
            Internal = 1<<5
        };
    };

    class EssentialRoot : public EssentialFeature
    {
    public:
        static const uint16_t ID = FeatureID::ROOT;
        virtual uint16_t getID() { return ID; }

        explicit EssentialRoot(hidpp::Device* device);

        feature_info getFeature(uint16_t feature_id);
        std::tuple<uint8_t, uint8_t> getVersion();
    };
}}}

#endif //LOGID_BACKEND_HIDPP20_FEATURE_ROOT_H