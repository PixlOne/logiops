#ifndef LOGID_BACKEND_ERROR_H
#define LOGID_BACKEND_ERROR_H

#include <stdexcept>

namespace logid {
namespace backend {
class TimeoutError: public std::exception
{
public:
    TimeoutError() = default;
    virtual const char* what() noexcept;
};
}}

#endif //LOGID_BACKEND_ERROR_H