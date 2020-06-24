#include "Error.h"

const char *logid::backend::TimeoutError::what() const noexcept
{
    return "Device timed out";
}
