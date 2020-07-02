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

#ifndef LOGID_FEATURES_DEVICEFEATURE_H
#define LOGID_FEATURES_DEVICEFEATURE_H

namespace logid {
    class Device;
namespace features
{
    class DeviceFeature
    {
    public:
        explicit DeviceFeature(Device* dev) : _device (dev)
        {
        }
        virtual void configure() = 0;
        virtual void listen() = 0;
        class Config
        {
        public:
            explicit Config(Device* dev) : _device (dev)
            {
            }
        protected:
            virtual const std::string configPath() = 0;
            Device* _device;
            std::string root_setting;
        };
    private:
        Device* _device;
    };
}}

#endif //LOGID_FEATURES_DEVICEFEATURE_H
