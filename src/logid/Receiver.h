#ifndef LOGID_RECEIVER_H
#define LOGID_RECEIVER_H

#include <string>

namespace logid
{
    class Receiver
    {
    public:
        Receiver(std::string path);
    private:
        std::string _path;
    };
}

#endif //LOGID_RECEIVER_H