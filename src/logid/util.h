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

#ifndef LOGID_UTIL_H
#define LOGID_UTIL_H

#include <string>

namespace logid
{
    enum LogLevel
    {
        RAWREPORT,
        DEBUG,
        INFO,
        WARN,
        ERROR
    };

    extern LogLevel global_verbosity;

    void log_printf(LogLevel level, const char* format, ...);

    const char* level_prefix(LogLevel level);

    /*
    Direction getDirection(int x, int y);
    Direction stringToDirection(std::string s);
    GestureMode stringToGestureMode(std::string s);
    Action stringToAction(std::string s);
     */
    LogLevel stringToLogLevel(std::string s);
}

#endif //LOGID_UTIL_H