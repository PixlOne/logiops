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

#ifndef LOGID_CONFIGURATION_H
#define LOGID_CONFIGURATION_H

#include <map>
#include <libconfig.h++>
#include <memory>
#include <chrono>

#define LOGID_DEFAULT_RAWDEVICE_TIMEOUT std::chrono::seconds(2)
#define LOGID_DEFAULT_WORKER_COUNT 2

namespace logid
{
    class Configuration
    {
    public:
        explicit Configuration(const std::string& config_file);
        Configuration() = default;
        libconfig::Setting& getSetting(std::string path);
        std::string getDevice(std::string name);

        class DeviceNotFound : public std::exception
        {
        public:
            explicit DeviceNotFound(std::string name);
            const char* what() const noexcept override;
        private:
            std::string _name;
        };

        std::chrono::milliseconds ioTimeout() const;
        int workerCount() const;
    private:
        std::map<std::string, std::string> _device_paths;
        std::chrono::milliseconds _io_timeout;
        int _worker_threads;
        libconfig::Config _config;
    };

    extern std::shared_ptr<Configuration> global_config;
}

#endif //LOGID_CONFIGURATION_H
