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

#include <Device.h>
#include <DeviceManager.h>
#include <features/SmartShift.h>
#include <features/DPI.h>
#include <features/RemapButton.h>
#include <features/HiresScroll.h>
#include <features/DeviceStatus.h>
#include <features/ThumbWheel.h>
#include <backend/hidpp20/features/Reset.h>
#include <util/log.h>
#include <thread>
#include <utility>
#include <ipc_defs.h>

using namespace logid;
using namespace logid::backend;

DeviceNickname::DeviceNickname(const std::shared_ptr<DeviceManager>& manager) :
        _nickname(manager->newDeviceNickname()), _manager(manager) {
}

DeviceNickname::operator std::string() const {
    return std::to_string(_nickname);
}

DeviceNickname::~DeviceNickname() {
    if (auto manager = _manager.lock()) {
        std::lock_guard<std::mutex> lock(manager->_nick_lock);
        manager->_device_nicknames.erase(_nickname);
    }
}

namespace logid {
    class DeviceWrapper : public Device {
    public:
        template<typename... Args>
        explicit DeviceWrapper(Args... args) : Device(std::forward<Args>(args)...) {}
    };
}

std::shared_ptr<Device> Device::make(
        std::string path, backend::hidpp::DeviceIndex index,
        std::shared_ptr<DeviceManager> manager) {
    auto ret = std::make_shared<DeviceWrapper>(std::move(path),
                                               index,
                                               std::move(manager));
    ret->_self = ret;
    ret->_ipc_node->manage(ret);
    ret->_ipc_interface = ret->_ipc_node->make_interface<IPC>(ret.get());
    return ret;
}

std::shared_ptr<Device> Device::make(
        std::shared_ptr<backend::raw::RawDevice> raw_device,
        backend::hidpp::DeviceIndex index,
        std::shared_ptr<DeviceManager> manager) {
    auto ret = std::make_shared<DeviceWrapper>(std::move(raw_device),
                                               index,
                                               std::move(manager));
    ret->_self = ret;
    ret->_ipc_node->manage(ret);
    ret->_ipc_interface = ret->_ipc_node->make_interface<IPC>(ret.get());
    return ret;
}

std::shared_ptr<Device> Device::make(
        Receiver* receiver, backend::hidpp::DeviceIndex index,
        std::shared_ptr<DeviceManager> manager) {
    auto ret = std::make_shared<DeviceWrapper>(receiver, index, std::move(manager));
    ret->_self = ret;
    ret->_ipc_node->manage(ret);
    ret->_ipc_interface = ret->_ipc_node->make_interface<IPC>(ret.get());
    return ret;
}

Device::Device(std::string path, backend::hidpp::DeviceIndex index,
               const std::shared_ptr<DeviceManager>& manager) :
        _hidpp20(path, index, manager,
                 manager->config()->io_timeout.value_or(defaults::io_timeout)),
        _path(std::move(path)), _index(index),
        _config(_getConfig(manager, _hidpp20.name())),
        _receiver(nullptr),
        _manager(manager),
        _nickname(manager),
        _ipc_node(manager->devicesNode()->make_child(_nickname)),
        _awake(ipcgull::property_readable, true) {
    _init();
}

Device::Device(std::shared_ptr<backend::raw::RawDevice> raw_device,
               hidpp::DeviceIndex index, const std::shared_ptr<DeviceManager>& manager) :
        _hidpp20(std::move(raw_device), index,
                 manager->config()->io_timeout.value_or(defaults::io_timeout)),
        _path(raw_device->rawPath()), _index(index),
        _config(_getConfig(manager, _hidpp20.name())),
        _receiver(nullptr),
        _manager(manager),
        _nickname(manager),
        _ipc_node(manager->devicesNode()->make_child(_nickname)),
        _awake(ipcgull::property_readable, true) {
    _init();
}

Device::Device(Receiver* receiver, hidpp::DeviceIndex index,
               const std::shared_ptr<DeviceManager>& manager) :
        _hidpp20(receiver->rawReceiver(), index,
                 manager->config()->io_timeout.value_or(
                         defaults::io_timeout)),
        _path(receiver->path()), _index(index),
        _config(_getConfig(manager, _hidpp20.name())),
        _receiver(receiver),
        _manager(manager),
        _nickname(manager),
        _ipc_node(manager->devicesNode()->make_child(_nickname)),
        _awake(ipcgull::property_readable, true) {
    _init();
}

void Device::_init() {
    logPrintf(INFO, "Device found: %s on %s:%d", name().c_str(),
              hidpp20().devicePath().c_str(), _index);

    _profile = _config.profiles.find(_config.default_profile);
    if (_profile == _config.profiles.end())
        _profile = _config.profiles.insert({_config.default_profile, {}}).first;

    _addFeature<features::DPI>("dpi");
    _addFeature<features::SmartShift>("smartshift");
    _addFeature<features::HiresScroll>("hiresscroll");
    _addFeature<features::RemapButton>("remapbutton");
    _addFeature<features::DeviceStatus>("devicestatus");
    _addFeature<features::ThumbWheel>("thumbwheel");

    _makeResetMechanism();
    reset();

    for (auto& feature: _features) {
        feature.second->configure();
        feature.second->listen();
    }
}

std::string Device::name() {
    return _hidpp20.name();
}

uint16_t Device::pid() {
    return _hidpp20.pid();
}

void Device::sleep() {
    std::lock_guard<std::mutex> lock(_state_lock);
    if (_awake) {
        logPrintf(INFO, "%s:%d fell asleep.", _path.c_str(), _index);
        _awake = false;
        _ipc_interface->notifyStatus();
    }
}

void Device::wakeup() {
    std::lock_guard<std::mutex> lock(_state_lock);
    logPrintf(INFO, "%s:%d woke up.", _path.c_str(), _index);

    reset();

    for (auto& feature: _features)
        feature.second->configure();

    if (!_awake) {
        _awake = true;
        _ipc_interface->notifyStatus();
    }
}

void Device::reset() {
    if (_reset_mechanism)
        (*_reset_mechanism)();
    else
        logPrintf(DEBUG, "%s:%d tried to reset, but no reset mechanism was "
                         "available.", _path.c_str(), _index);
}

std::shared_ptr<InputDevice> Device::virtualInput() const {
    if (auto manager = _manager.lock()) {
        return manager->virtualInput();
    } else {
        logPrintf(ERROR, "Device manager lost");
        logPrintf(ERROR,
                  "Fatal error occurred, file a bug report,"
                  " the program will now exit.");
        std::terminate();
    }
}

std::shared_ptr<ipcgull::node> Device::ipcNode() const {
    return _ipc_node;
}

/*config::Device& Device::config()
{
    return _config;
}*/

config::Profile& Device::activeProfile() {
    return _profile->second;
}

const config::Profile& Device::activeProfile() const {
    return _profile->second;
}

hidpp20::Device& Device::hidpp20() {
    return _hidpp20;
}

void Device::_makeResetMechanism() {
    try {
        hidpp20::Reset reset(&_hidpp20);
        _reset_mechanism = std::make_unique<std::function<void()>>(
                [dev = &this->_hidpp20] {
                    hidpp20::Reset reset(dev);
                    reset.reset(reset.getProfile());
                });
    } catch (hidpp20::UnsupportedFeature& e) {
        // Reset unsupported, ignore.
    }
}

Device::IPC::IPC(Device* device) :
        ipcgull::interface(
                SERVICE_ROOT_NAME ".Device",
                {},
                {
                        {"Name",           ipcgull::property<std::string>(
                                ipcgull::property_readable, device->name())},
                        {"ProductID",      ipcgull::property<uint16_t>(
                                ipcgull::property_readable, device->pid())},
                        {"Active",         device->_awake},
                        {"DefaultProfile", device->_config.default_profile}
                }, {
                        {"StatusChanged",
                         ipcgull::signal::make_signal<bool>({"active"})}
                }), _device(*device) {
}

void Device::IPC::notifyStatus() const {
    emit_signal("StatusChanged", (bool) (_device._awake));
}

config::Device& Device::_getConfig(
        const std::shared_ptr<DeviceManager>& manager,
        const std::string& name) {
    static std::mutex config_mutex;
    std::lock_guard<std::mutex> lock(config_mutex);
    auto& devices = manager->config()->devices;
    if (!devices.has_value())
        devices = decltype(config::Config::devices)();

    if (!devices.value().count(name)) {
        devices.value().emplace(name, config::Device());
    }

    auto& device = devices.value().at(name);
    if (std::holds_alternative<config::Profile>(device)) {
        config::Device d;
        d.profiles["default"] = std::get<config::Profile>(device);
        d.default_profile = "default";
        device = std::move(d);
    }

    auto& conf = std::get<config::Device>(device);
    if (conf.profiles.empty()) {
        conf.profiles["default"] = {};
        conf.default_profile = "default";
    }

    return conf;
}
