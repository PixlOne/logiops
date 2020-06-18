#ifndef LOGID_BACKEND_HIDPP10_ERROR_H
#define LOGID_BACKEND_HIDPP10_ERROR_H

#include <cstdint>

namespace logid {
namespace backend {
namespace hidpp10 {
    static constexpr uint8_t ErrorID = 0x8f;

    class Error: public std::exception
    {
    public:
        enum ErrorCode: uint8_t
        {
            Success = 0x00,
            InvalidSubID = 0x01,
            InvalidAddress = 0x02,
            InvalidValue = 0x03,
            ConnectFail = 0x04,
            TooManyDevices = 0x05,
            AlreadyExists = 0x06,
            Busy = 0x07,
            UnknownDevice = 0x08,
            ResourceError = 0x09,
            RequestUnavailable = 0x0A,
            InvalidParameterValue = 0x0B,
            WrongPINCode = 0x0C
        };

        Error(uint8_t code);

        virtual const char* what() const noexcept;
        uint8_t code() const noexcept;

    private:
        uint8_t _code;
    };
}}}

#endif //LOGID_BACKEND_HIDPP10_ERROR_H