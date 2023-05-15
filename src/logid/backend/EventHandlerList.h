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
#include <atomic>

template <class T>
class EventHandlerLock;

template <class T>
class EventHandlerList {
public:
    typedef std::list<std::pair<typename T::EventHandler, std::atomic_bool>> list_t;
    typedef typename list_t::iterator iterator_t;
private:
    list_t list;
    std::shared_mutex mutex;
    std::shared_mutex add_mutex;

    void cleanup() {
        std::unique_lock lock(mutex, std::try_to_lock);
        if (lock.owns_lock()) {
            std::list<iterator_t> to_remove;
            for (auto it = list.begin(); it != list.end(); ++it) {
                if (!it->second)
                    to_remove.push_back(it);
            }

            for(auto& it : to_remove)
                list.erase(it);
        }
    }
public:
    iterator_t add(typename T::EventHandler handler) {
        std::unique_lock add_lock(add_mutex);
        list.emplace_front(std::move(handler), true);
        return list.begin();
    }

    void remove(iterator_t iterator) {
        std::unique_lock lock(mutex, std::try_to_lock);
        if (lock.owns_lock()) {
            std::unique_lock add_lock(add_mutex);
            list.erase(iterator);
        } else {
            iterator->second = false;
        }
    }

    template <typename Arg>
    void run_all(Arg arg) {
        cleanup();
        std::shared_lock lock(mutex);
        std::shared_lock add_lock(add_mutex);
        for (auto& handler : list) {
            add_lock.unlock();
            if (handler.second) {
                if (handler.first.condition(arg))
                    handler.first.callback(arg);
            }
            add_lock.lock();
        }
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
        if (this != &o) {
            if (auto list = _list.lock()) {
                this->_list.reset();
                list->remove(_iterator);
            }

            this->_list = o._list;
            o._list.reset();
            this->_iterator = o._iterator;
        }

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
