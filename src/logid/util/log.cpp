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

#include <util/log.h>
#include <cstdio>
#include <cstdarg>
#include <algorithm>
#include <stdexcept>

using namespace logid;

void logid::logPrintf(LogLevel level, const char* format, ...) {
    if (global_loglevel > level) return;

    va_list vargs;
    va_start(vargs, format);

    FILE* stream = stdout;
    if (level == ERROR || level == WARN)
        stream = stderr;

    fprintf(stream, "[%s] ", levelPrefix(level));
    vfprintf(stream, format, vargs);
    fprintf(stream, "\n");
}

const char* logid::levelPrefix(LogLevel level) {
    switch (level) {
        case RAWREPORT:
            return "RAWREPORT";
        case DEBUG:
            return "DEBUG";
        case INFO:
            return "INFO";
        case WARN:
            return "WARN";
        case ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}


LogLevel logid::toLogLevel(std::string s) {
    std::string original_str = s;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    if (s == "rawreport")
        return RAWREPORT;
    if (s == "debug")
        return DEBUG;
    if (s == "info")
        return INFO;
    if (s == "warn" || s == "warning")
        return WARN;
    if (s == "error")
        return ERROR;

    throw std::invalid_argument(original_str + " is an invalid log level.");
}