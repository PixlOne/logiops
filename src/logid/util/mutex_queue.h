#ifndef MUTEX_QUEUE_H
#define MUTEX_QUEUE_H

#include <queue>
#include <mutex>

template<typename data>
class mutex_queue
{
public:
    mutex_queue<data>() = default;
    bool empty()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.empty();
    }
    data& front()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.front();
    }
    void push(const data& _data)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(_data);
    }
    void pop()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.pop();
    }
private:
    std::queue<data> _queue;
    std::mutex _mutex;
};

#endif //MUTEX_QUEUE_H