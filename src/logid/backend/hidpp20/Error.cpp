#include <cassert>
#include <cstring>
#include "Error.h"

using namespace logid::backend::hidpp20;

Error::Error(uint8_t code) : _code (code)
{
    assert(_code != NoError);
}

const char* Error::what() const noexcept
{
    switch(_code)
    {
        case NoError:
            return "No error";
        case Unknown:
            return "Unknown";
        case InvalidArgument:
            return "Invalid argument";
        case OutOfRange:
            return "Out of range";
        case HardwareError:
            return "Hardware error";
        case LogitechInternal:
            return "Logitech internal feature";
        case InvalidFeatureIndex:
            return "Invalid feature index";
        case InvalidFunctionID:
            return "Invalid function ID";
        case Busy:
            return "Busy";
        case Unsupported:
            return "Unsupported";
        case UnknownDevice:
            return "Unknown device";
        default:
            return std::string("Unknown error code " + std::to_string(_code)).c_str();
    }
}

uint8_t Error::code() const noexcept
{
    return _code;
}