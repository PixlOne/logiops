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
#ifndef LOGID_FEATURE_HIRESSCROLL_H
#define LOGID_FEATURE_HIRESSCROLL_H

#include "../backend/hidpp20/features/HiresScroll.h"
#include "DeviceFeature.h"

namespace logid {
namespace features
{
    class HiresScroll : public DeviceFeature
    {
    public:
        explicit HiresScroll(Device* dev);
        virtual void configure();
        virtual void listen();

        class Config : public DeviceFeature::Config
        {
        public:
            explicit Config(Device* dev);
            uint8_t getMode() const;
            uint8_t getMask() const;
        protected:
            uint8_t _mode;
            uint8_t _mask;
        };
    private:
        backend::hidpp20::HiresScroll _hires_scroll;
        Config _config;
    };
}}

#endif //LOGID_FEATURE_HIRESSCROLL_H
