#include "Error.h"

const char *logid::backend::TimeoutError::what() noexcept
{
    return "Device timed out";
}
