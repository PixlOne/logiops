#ifndef LOGID_BACKEND_ERROR_H
#define LOGID_BACKEND_ERROR_H

#include <stdexcept>

namespace logid {
namespace backend {
class TimeoutError: public std::exception
{
public:
    TimeoutError() = default;
    const char* what() const noexcept override;
};
}}

#endif //LOGID_BACKEND_ERROR_H