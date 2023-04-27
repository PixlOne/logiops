/*
 * Copyright 2022 PixlOne
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
#include <cassert>
#include "IOMonitor.h"

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
    std::lock_guard<std::mutex> ctl_lock(_ctl_lock);
    _stop();

    if (_event_fd >= 0)
        close(_event_fd);

    if (_epoll_fd >= 0)
        close(_epoll_fd);
}

void IOMonitor::_listen() {
    std::lock_guard<std::mutex> run_lock(_run_lock);
    std::vector<struct epoll_event> events;

    _is_running = true;

    while (_is_running) {
        {
            std::unique_lock<std::mutex> lock(_interrupt_lock);
            _interrupt_cv.wait(lock, [this]() {
                return !(bool) _interrupting;
            });

            if (!_is_running)
                break;
        }

        std::lock_guard<std::mutex> io_lock(_io_lock);
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
    _interrupt();
    _is_running = false;
    _continue();
    _io_thread->join();
}

[[maybe_unused]]
bool IOMonitor::_running() const {
    std::unique_lock<std::mutex> run_lock(_run_lock, std::try_to_lock);
    return !run_lock.owns_lock() || _is_running;
}

void IOMonitor::add(int fd, IOHandler handler) {
    std::lock_guard<std::mutex> lock(_ctl_lock);
    _interrupt();

    struct epoll_event event{};
    event.events = EPOLLIN | EPOLLHUP | EPOLLERR;
    event.data.fd = fd;

    // TODO: EPOLL_CTL_MOD
    if (_fds.contains(fd)) {
        _continue();
        throw std::runtime_error("duplicate io fd");
    }

    if (::epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event)) {
        _continue();
        throw std::system_error(errno, std::generic_category());
    }
    _fds.emplace(fd, std::move(handler));

    _continue();
}

void IOMonitor::remove(int fd) noexcept {
    std::lock_guard<std::mutex> lock(_ctl_lock);
    _interrupt();
    std::lock_guard<std::mutex> io_lock(_io_lock);

    ::epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    _fds.erase(fd);

    _continue();
}

void IOMonitor::_interrupt() noexcept {
    std::unique_lock<std::mutex> run_lock(_run_lock, std::try_to_lock);

    _interrupting = true;

    uint64_t counter = 1;
    ssize_t ret = ::write(_event_fd, &counter, sizeof(counter));
    assert(ret == sizeof(counter));

    // Wait for the IO monitor to _stop
    std::lock_guard<std::mutex> io_lock(_io_lock);

}

void IOMonitor::_continue() noexcept {
    std::unique_lock<std::mutex> run_lock(_run_lock, std::try_to_lock);

    uint64_t counter;
    ssize_t ret = ::read(_event_fd, &counter, sizeof(counter));

    assert(ret != -1);

    if (counter == 1) {
        _interrupting = false;
        _interrupt_cv.notify_all();
    }
}
