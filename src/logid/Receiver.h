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

#ifndef LOGID_RECEIVER_H
#define LOGID_RECEIVER_H

#include <string>
#include <Device.h>
#include <backend/hidpp10/ReceiverMonitor.h>

namespace logid {
    class ReceiverNickname {
    public:
        explicit ReceiverNickname(const std::shared_ptr<DeviceManager>& manager);

        ReceiverNickname() = delete;

        ReceiverNickname(const ReceiverNickname&) = delete;

        ~ReceiverNickname();

        operator std::string() const;

    private:
        const int _nickname;
        const std::weak_ptr<DeviceManager> _manager;
    };

    class Receiver : public backend::hidpp10::ReceiverMonitor,
                     public ipcgull::object {
    public:
        typedef std::map<backend::hidpp::DeviceIndex, std::shared_ptr<Device>>
                DeviceList;

        ~Receiver() noexcept override;

        static std::shared_ptr<Receiver> make(
                const std::string& path,
                const std::shared_ptr<DeviceManager>& manager);

        [[nodiscard]] const std::string& path() const;

        std::shared_ptr<backend::hidpp10::Receiver> rawReceiver();

        [[nodiscard]] const DeviceList& devices() const;

        [[nodiscard]] std::vector<std::tuple<int, uint16_t, std::string, uint32_t>>
        pairedDevices() const;

        void startPair(uint8_t timeout);

        void stopPair();

        void unpair(int device);

    protected:
        Receiver(const std::string& path,
                 const std::shared_ptr<DeviceManager>& manager);

        void addDevice(backend::hidpp::DeviceConnectionEvent event) override;

        void removeDevice(backend::hidpp::DeviceIndex index) override;

        void pairReady(const backend::hidpp10::DeviceDiscoveryEvent& event,
                       const std::string& passkey) override;

    private:
        std::mutex _devices_change;
        DeviceList _devices;
        std::string _path;
        std::weak_ptr<DeviceManager> _manager;

        const ReceiverNickname _nickname;
        std::shared_ptr<ipcgull::node> _ipc_node;

        class IPC : public ipcgull::interface {
        public:
            explicit IPC(Receiver* receiver);
        };

        std::shared_ptr<ipcgull::interface> _ipc_interface;
    };
}

#endif //LOGID_RECEIVER_H