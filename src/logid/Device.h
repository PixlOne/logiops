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

#ifndef LOGID_DEVICE_H
#define LOGID_DEVICE_H

#include <features/DeviceFeature.h>
#include <backend/hidpp20/Device.h>
#include <backend/hidpp/defs.h>
#include <ipcgull/node.h>
#include <ipcgull/interface.h>
#include <Configuration.h>

namespace logid {
    class DeviceManager;

    class Device;

    class Receiver;

    class InputDevice;

    class DeviceNickname {
    public:
        explicit DeviceNickname(const std::shared_ptr<DeviceManager>& manager);

        DeviceNickname() = delete;

        DeviceNickname(const DeviceNickname&) = delete;

        ~DeviceNickname();

        operator std::string() const;

    private:
        const int _nickname;
        const std::weak_ptr<DeviceManager> _manager;
    };

    /* TODO: Implement HID++ 1.0 support
     * Currently, the logid::Device class has a hardcoded requirement
     * for an HID++ 2.0 device.
     */
    class Device : public ipcgull::object {
    public:
        std::string name();

        uint16_t pid();

        [[nodiscard]] config::Profile& activeProfile();

        [[nodiscard]] std::vector<std::string> getProfiles() const;

        void setProfile(const std::string& profile);

        void setProfileDelayed(const std::string& profile);

        void removeProfile(const std::string& profile);

        void clearProfile(const std::string& profile);

        backend::hidpp20::Device& hidpp20();

        static std::shared_ptr<Device> make(
                std::string path,
                backend::hidpp::DeviceIndex index,
                std::shared_ptr<DeviceManager> manager);

        static std::shared_ptr<Device> make(
                std::shared_ptr<backend::raw::RawDevice> raw_device,
                backend::hidpp::DeviceIndex index,
                std::shared_ptr<DeviceManager> manager);

        static std::shared_ptr<Device> make(
                Receiver* receiver,
                backend::hidpp::DeviceIndex index,
                std::shared_ptr<DeviceManager> manager);

        void wakeup();

        void sleep();

        void reconfigure();

        void reset();

        [[nodiscard]] std::shared_ptr<InputDevice> virtualInput() const;

        [[nodiscard]] std::shared_ptr<ipcgull::node> ipcNode() const;

        template<typename T>
        std::shared_ptr<T> getFeature(const std::string& name) {
            auto it = _features.find(name);
            if (it == _features.end())
                return nullptr;
            try {
                return std::dynamic_pointer_cast<T>(it->second);
            } catch (std::bad_cast& e) {
                return nullptr;
            }
        }

        Device(const Device&) = delete;

        Device(Device&&) = delete;

    private:
        friend class DeviceWrapper;

        Device(std::string path, backend::hidpp::DeviceIndex index,
               const std::shared_ptr<DeviceManager>& manager);

        Device(std::shared_ptr<backend::raw::RawDevice> raw_device,
               backend::hidpp::DeviceIndex index,
               const std::shared_ptr<DeviceManager>& manager);

        Device(Receiver* receiver, backend::hidpp::DeviceIndex index,
               const std::shared_ptr<DeviceManager>& manager);

        static config::Device& _getConfig(
                const std::shared_ptr<DeviceManager>& manager,
                const std::string& name);

        void _init();

        /* Adds a feature without calling an error if unsupported */
        template<typename T>
        void _addFeature(std::string name) {
            try {
                _features.emplace(name, features::DeviceFeature::make<T>(this));
            } catch (features::UnsupportedFeature& e) {
            }
        }

        std::shared_ptr<backend::hidpp20::Device> _hidpp20;
        std::string _path;
        backend::hidpp::DeviceIndex _index;
        std::map<std::string, std::shared_ptr<features::DeviceFeature>> _features;

        config::Device& _config;
        mutable std::shared_mutex _profile_mutex;
        ipcgull::property<std::string> _profile_name;
        std::map<std::string, config::Profile>::iterator _profile;

        const std::weak_ptr<DeviceManager> _manager;

        void _makeResetMechanism();

        std::unique_ptr<std::function<void()>> _reset_mechanism;

        const DeviceNickname _nickname;
        std::shared_ptr<ipcgull::node> _ipc_node;

        class IPC : public ipcgull::interface {
        private:
            Device& _device;
        public:
            explicit IPC(Device* device);

            void notifyStatus() const;
        };

        ipcgull::property<bool> _awake;
        std::mutex _state_lock;

        std::weak_ptr<Device> _self;

        std::shared_ptr<IPC> _ipc_interface;
    };
}

#endif //LOGID_DEVICE_H
