#include "Error.h"

using namespace logid::backend::dj;

Error::Error(uint8_t code) : _code (code)
{
}

const char* Error::what() const noexcept
{
    switch(_code)
    {
        case Unknown:
            return "Unknown";
        case KeepAliveTimeout:
            return "Keep-alive timeout";
        default:
            return std::string("Reserved error code " + std::to_string(_code))
                .c_str();
    }
}