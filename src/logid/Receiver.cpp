#include "Receiver.h"
#include "util.h"

using namespace logid;

Receiver::Receiver(std::string path) : _path (path)
{
    log_printf(DEBUG, "logid::Receiver created on %s", path.c_str());
}