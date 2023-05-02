/*
 * Copyright 2019-2023 PixlOne
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


#ifndef LOGID_EVENTHANDLERLIST_H
#define LOGID_EVENTHANDLERLIST_H

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <list>

template <class T>
class EventHandlerLock;

template <class T>
struct EventHandlerList {
    typedef std::list<typename T::EventHandler> list_t;
    typedef list_t::const_iterator iterator_t;

    std::list<typename T::EventHandler> list;
    mutable std::shared_mutex mutex;

    void remove(iterator_t iterator) {
        std::unique_lock lock(mutex);
        list.erase(iterator);
    }
};

template <class T>
class EventHandlerLock {
    typedef EventHandlerList<T> list_t;
    typedef typename list_t::iterator_t iterator_t;

    friend T;

    std::weak_ptr<list_t> _list;
    iterator_t _iterator;

    EventHandlerLock(const std::shared_ptr<list_t>& list, iterator_t iterator) :
                     _list (list), _iterator (iterator) {
    }
public:
    EventHandlerLock() = default;

    EventHandlerLock(const EventHandlerLock&) = delete;

    EventHandlerLock(EventHandlerLock&& o) noexcept : _list (o._list), _iterator (o._iterator) {
        o._list.reset();
    }

    EventHandlerLock& operator=(const EventHandlerLock&) = delete;

    EventHandlerLock& operator=(EventHandlerLock&& o) noexcept {
        this->_list = o._list;
        this->_iterator = o._iterator;
        o._list.reset();

        return *this;
    }

    ~EventHandlerLock() {
        if(auto list = _list.lock())
            list->remove(_iterator);
    }

    [[nodiscard]] bool empty() const noexcept {
        return _list.expired();
    }
};

#endif //LOGID_EVENTHANDLERLIST_H
