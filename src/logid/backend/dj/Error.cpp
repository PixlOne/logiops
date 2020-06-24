#include "Error.h"

using namespace logid::backend::dj;

Error::Error(uint8_t code) : _code (code)
{
}

const char* Error::what() const noexcept
{
    switch(_code) {
    case Unknown:
        return "Unknown";
    case KeepAliveTimeout:
        return "Keep-alive timeout";
    default:
        return "Reserved";
    }
}

uint8_t Error::code() const noexcept
{
    return _code;
}