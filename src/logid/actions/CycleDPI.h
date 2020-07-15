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
#ifndef LOGID_ACTION_CYCLEDPI_H
#define LOGID_ACTION_CYCLEDPI_H

#include <libconfig.h++>
#include "Action.h"
#include "../features/DPI.h"

namespace logid {
namespace actions {
    class CycleDPI : public Action
    {
    public:
        explicit CycleDPI(Device* device, libconfig::Setting& setting);

        virtual void press();
        virtual void release();

        virtual uint8_t reprogFlags() const;

    class Config : public Action::Config
    {
    public:
        Config(Device* device, libconfig::Setting& setting);
        uint16_t nextDPI();
        bool empty() const;
        uint8_t sensor() const;
    private:
        std::size_t _current_index;
        std::vector<uint16_t> _dpis;
        uint8_t _sensor;
    };

    protected:
        Config _config;
        std::shared_ptr<features::DPI> _dpi;
    };
}}

#endif //LOGID_ACTION_CYCLEDPI_H