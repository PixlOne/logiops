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
#include <backend/raw/IOMonitor.h>
#include <cassert>
#include <optional>

extern "C"
{
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
}

using namespace logid::backend::raw;

IOHandler::IOHandler(std::function<void()> r,
                     std::function<void()> hup,
                     std::function<void()> err) :
        read(std::move(r)),
        hangup(std::move(hup)),
        error(std::move(err)) {
}

class IOMonitor::io_lock {
    std::optional<std::unique_lock<std::mutex>> _lock;
    IOMonitor* _io_monitor;
    const uint64_t counter = 1;

public:
    explicit io_lock(IOMonitor* io_monitor) : _io_monitor(io_monitor) {
        _io_monitor->_interrupting = true;
        [[maybe_unused]] ssize_t ret = ::write(_io_monitor->_event_fd, &counter, sizeof(counter));
        assert(ret == sizeof(counter));
        _lock.emplace(_io_monitor->_run_mutex);
    }

    io_lock(const io_lock&) = delete;

    io_lock& operator=(const io_lock&) = delete;

    io_lock(io_lock&& o) noexcept: _lock(std::move(o._lock)), _io_monitor(o._io_monitor) {
        o._lock.reset();
        o._io_monitor = nullptr;
    }

    io_lock& operator=(io_lock&& o) noexcept {
        if (this != &o) {
            _lock = std::move(o._lock);
            _io_monitor = o._io_monitor;
            o._lock.reset();
            o._io_monitor = nullptr;
        }

        return *this;
    }

    ~io_lock() noexcept {
        if (_lock && _io_monitor) {
            uint64_t buf{};
            [[maybe_unused]] const ssize_t ret = ::read(
                    _io_monitor->_event_fd, &buf, sizeof(counter));

            assert(ret != -1);

            if (buf == counter) {
                _io_monitor->_interrupting = false;
                _io_monitor->_interrupt_cv.notify_one();
            }
        }
    }
};

IOMonitor::IOMonitor() : _epoll_fd(epoll_create1(0)),
                         _event_fd(eventfd(0, EFD_NONBLOCK)) {
    if (_epoll_fd < 0) {
        if (_event_fd >= 0)
            close(_event_fd);
        throw std::runtime_error("failed to create epoll fd");
    }

    if (_event_fd < 0) {
        close(_epoll_fd);
        throw std::runtime_error("failed to create event fd");
    }

    struct epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = _event_fd;

    if (::epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _event_fd, &event)) {
        throw std::system_error(errno, std::generic_category());
    }

    _fds.emplace(std::piecewise_construct, std::forward_as_tuple(_event_fd),
                 std::forward_as_tuple([]() {}, []() {
                     throw std::runtime_error("event_fd hangup");
                 }, []() {
                     throw std::runtime_error("event_fd error");
                 }));

    _io_thread = std::make_unique<std::thread>([this]() {
        _listen();
    });
}

IOMonitor::~IOMonitor() noexcept {
    _stop();

    if (_event_fd >= 0)
        close(_event_fd);

    if (_epoll_fd >= 0)
        close(_epoll_fd);
}

void IOMonitor::_listen() {
    std::unique_lock lock(_run_mutex);
    std::vector<struct epoll_event> events;

    _is_running = true;

    while (_is_running) {
        if (_interrupting) {
            _interrupt_cv.wait(lock, [this]() {
                return !_interrupting;
            });

            if (!_is_running)
                break;
        }

        if (events.size() != _fds.size())
            events.resize(_fds.size());
        int ev_count = ::epoll_wait(_epoll_fd, events.data(), (int) events.size(), -1);
        for (int i = 0; i < ev_count; ++i) {
            const auto& handler = _fds.at(events[i].data.fd);
            if (events[i].events & EPOLLIN)
                handler.read();
            if (events[i].events & EPOLLHUP)
                handler.hangup();
            if (events[i].events & EPOLLERR)
                handler.error();
        }
    }
}

void IOMonitor::_stop() noexcept {
    {
        [[maybe_unused]] const io_lock lock(this);
        _is_running = false;
    }
    _io_thread->join();
}

void IOMonitor::add(int fd, IOHandler handler) {
    [[maybe_unused]] const io_lock lock(this);

    struct epoll_event event{};
    event.events = EPOLLIN | EPOLLHUP | EPOLLERR;
    event.data.fd = fd;

    // TODO: EPOLL_CTL_MOD
    if (_fds.contains(fd))
        throw std::runtime_error("duplicate io fd");

    if (::epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event))
        throw std::system_error(errno, std::generic_category());
    _fds.emplace(fd, std::move(handler));
}

void IOMonitor::remove(int fd) noexcept {
    [[maybe_unused]] const io_lock lock(this);

    ::epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    _fds.erase(fd);
}