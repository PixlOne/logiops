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

#ifndef LOGID_FEATURES_DEVICEFEATURE_H
#define LOGID_FEATURES_DEVICEFEATURE_H

#include <memory>
#include <string>

namespace logid {
    class Device;
}

namespace logid::config {
    struct Profile;
}

namespace logid::features {
    class UnsupportedFeature : public std::exception {
    public:
        UnsupportedFeature() = default;

        [[nodiscard]] const char* what() const noexcept override {
            return "Unsupported feature";
        }
    };

    template<typename T>
    class _featureWrapper : public T {
        friend class DeviceFeature;

    public:
        template<typename... Args>
        explicit _featureWrapper(Args... args) : T(std::forward<Args>(args)...) {}

        template<typename... Args>
        static std::shared_ptr<T> make(Args... args) {
            return std::make_shared<_featureWrapper>(std::forward<Args>(args)...);
        }
    };

    class DeviceFeature {
        std::weak_ptr<DeviceFeature> _self;
    public:
        virtual void configure() = 0;

        virtual void listen() = 0;

        virtual void setProfile(config::Profile& profile) = 0;

        virtual ~DeviceFeature() = default;

        DeviceFeature(const DeviceFeature&) = delete;

        DeviceFeature(DeviceFeature&&) = delete;

    protected:
        explicit DeviceFeature(Device* dev) : _device(dev) {}

        Device* _device;

        template<typename T>
        [[nodiscard]] std::weak_ptr<T> self() const {
            return std::dynamic_pointer_cast<T>(_self.lock());
        }

    public:
        template<typename T, typename... Args>
        static std::shared_ptr<T> make(Args... args) {
            auto feature = _featureWrapper<T>::make(std::forward<Args>(args)...);
            feature->_self = feature;

            return feature;
        }
    };
}

#endif //LOGID_FEATURES_DEVICEFEATURE_H
