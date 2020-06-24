#ifndef LOGID_BACKEND_DJ_ERROR_H
#define LOGID_BACKEND_DJ_ERROR_H

#include <cstdint>
#include <stdexcept>

namespace logid {
namespace backend {
namespace dj
{
    class Error : public std::exception
    {
    public:
        enum ErrorCode : uint8_t
        {
            Unknown = 0x00,
            KeepAliveTimeout = 0x01
        };

        explicit Error(uint8_t code);

        const char* what() const noexcept override;
        uint8_t code() const noexcept;

    private:
        uint8_t _code;
    };
}}}

#endif //LOGID_BACKEND_DJ_ERROR_H