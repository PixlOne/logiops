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

#ifndef LOGID_CONFIGURATION_H
#define LOGID_CONFIGURATION_H

#include <config/schema.h>
#include <ipcgull/interface.h>
#include <libconfig.h++>
#include <memory>
#include <chrono>
#include <set>

namespace logid {
    namespace defaults {
        static constexpr double io_timeout = 500;
        static constexpr int workers = 4;
        static constexpr int gesture_threshold = 50;
    }

    class Configuration : public config::Config {
    public:
        explicit Configuration(std::string config_file);

        Configuration();

        // Reloading is not safe, references will be invalidated
        //void reload();
        void save();

        class IPC : public ipcgull::interface {
        public:
            explicit IPC(Configuration* config);
        };

    private:
        std::string _config_file;
        libconfig::Config _config;
    };

}

#endif //LOGID_CONFIGURATION_H
