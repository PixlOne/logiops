/*
 * Copyright 2019-2020 PixlOne
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MUTEX_QUEUE_H
#define MUTEX_QUEUE_H

#include <queue>
#include <mutex>

template<typename data>
class mutex_queue
{
public:
    mutex_queue() = default;
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