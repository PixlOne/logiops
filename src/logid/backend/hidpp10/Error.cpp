#include <cassert>
#include <string>
#include "Error.h"

using namespace logid::backend::hidpp10;

Error::Error(uint8_t code): _code(code)
{
    assert(code != Success);
}

const char* Error::what() const noexcept
{
    switch(_code)
    {
        case Success:
            return "Success";
        case InvalidSubID:
            return "Invalid sub ID";
        case InvalidAddress:
            return "Invalid address";
        case InvalidValue:
            return "Invalid value";
        case ConnectFail:
            return "Connection failure";
        case TooManyDevices:
            return "Too many devices";
        case AlreadyExists:
            return "Already exists";
        case Busy:
            return "Busy";
        case UnknownDevice:
            return "Unknown device";
        case ResourceError:
            return "Resource error";
        case RequestUnavailable:
            return "Request unavailable";
        case InvalidParameterValue:
            return "Invalid parameter value";
        case WrongPINCode:
            return "Wrong PIN code";
        default:
            return std::string("Unknown error code " + std::to_string(_code)).c_str();
    }
}

uint8_t Error::code() const noexcept
{
    return _code;
}