#ifndef LOGID_UTIL_H
#define LOGID_UTIL_H

//#include "Actions.h"

namespace logid
{
    enum LogLevel
    {
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