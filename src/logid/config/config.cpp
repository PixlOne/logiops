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

#include <config/schema.h>
#include <util/log.h>

using namespace logid;

const char config::keys::name[] = "name";
const char config::keys::cid[] = "cid";
const char config::keys::direction[] = "direction";

void config::logError(const libconfig::Setting& setting, std::exception& e) {
    logPrintf(WARN, "Error at line %d: %s", setting.getSourceLine(), e.what());
}