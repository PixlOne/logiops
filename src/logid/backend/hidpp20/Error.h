#ifndef LOGID_BACKEND_HIDPP20_ERROR_H
#define LOGID_BACKEND_HIDPP20_ERROR_H

#include <stdexcept>
#include <cstdint>

namespace logid {
namespace backend {
namespace hidpp20 {
    static constexpr uint8_t ErrorID = 0xFF;

    class Error: public std::exception
    {
    public:
        enum ErrorCode: uint8_t {
            NoError = 0,
            Unknown = 1,
            InvalidArgument = 2,
            OutOfRange = 3,
            HardwareError = 4,
            LogitechInternal = 5,
            InvalidFeatureIndex = 6,
            InvalidFunctionID = 7,
            Busy = 8,
            Unsupported = 9,
            UnknownDevice = 10
        };

        Error(uint8_t code);

        virtual const char* what() const noexcept;
        uint8_t code() const noexcept;

    private:
        uint8_t _code;
    };
}}}

#endif //LOGID_BACKEND_HIDPP20_ERROR_H