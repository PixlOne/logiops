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

#ifndef LOGID_DEVICEMANAGER_H
#define LOGID_DEVICEMANAGER_H

#include <map>
#include <thread>
#include <mutex>
#include <ipcgull/node.h>
#include <ipcgull/interface.h>

#include "backend/raw/DeviceMonitor.h"
#include "backend/hidpp/Device.h"
#include "Device.h"
#include "Receiver.h"

namespace logid
{
    class workqueue;
    class InputDevice;

    class DeviceManager : public backend::raw::DeviceMonitor
    {
    public:
        static std::shared_ptr<DeviceManager> make(
                const std::shared_ptr<Configuration>& config,
                const std::shared_ptr<InputDevice>& virtual_input,
                const std::shared_ptr<ipcgull::server>& server);
        [[nodiscard]] std::shared_ptr<Configuration> config() const;
        [[nodiscard]] std::shared_ptr<InputDevice> virtualInput() const;
        [[nodiscard]] std::shared_ptr<const ipcgull::node> devicesNode() const;
        [[nodiscard]] std::shared_ptr<const ipcgull::node>
            receiversNode() const;

        void addExternalDevice(const std::shared_ptr<Device>& d);
        void removeExternalDevice(const std::shared_ptr<Device>& d);
    protected:
        void addDevice(std::string path) final;
        void removeDevice(std::string path) final;
    private:
        class DevicesIPC : public ipcgull::interface {
        public:
            DevicesIPC();
            void deviceAdded(const std::shared_ptr<Device>& d);
            void deviceRemoved(const std::shared_ptr<Device>& d);
        };

        class ReceiversIPC : public ipcgull::interface {
        public:
            ReceiversIPC();
            void receiverAdded(const std::shared_ptr<Receiver>& r);
            void receiverRemoved(const std::shared_ptr<Receiver>& r);
        };

        friend class _DeviceManager;
        DeviceManager(std::shared_ptr<Configuration> config,
                      std::shared_ptr<InputDevice> virtual_input,
                      std::shared_ptr<ipcgull::server> server);

        std::weak_ptr<DeviceManager> _self;
        std::shared_ptr<ipcgull::server> _server;
        std::shared_ptr<Configuration> _config;
        std::shared_ptr<InputDevice> _virtual_input;

        std::shared_ptr<ipcgull::node> _root_node;

        std::shared_ptr<ipcgull::node> _device_node;
        std::shared_ptr<ipcgull::node> _receiver_node;

        std::shared_ptr<DevicesIPC> _ipc_devices;
        std::shared_ptr<ReceiversIPC> _ipc_receivers;

        std::map<std::string, std::shared_ptr<Device>> _devices;
        std::map<std::string, std::shared_ptr<Receiver>> _receivers;

        friend class DeviceNickname;
        friend class ReceiverNickname;

        [[nodiscard]] int newDeviceNickname();
        [[nodiscard]] int newReceiverNickname();

        std::mutex _nick_lock;
        std::set<int> _device_nicknames;
        std::set<int> _receiver_nicknames;
    };

}

#endif //LOGID_DEVICEMANAGER_H
