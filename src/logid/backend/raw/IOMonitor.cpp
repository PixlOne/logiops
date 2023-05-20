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
#include <util/log.h>
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

    _fds.emplace(_event_fd, nullptr);

    _io_thread = std::make_unique<std::thread>([this]() {
        _listen();
    });
}

IOMonitor::~IOMonitor() noexcept {
    _stop();

    if (_event_fd >= 0)
        ::close(_event_fd);

    if (_epoll_fd >= 0)
        ::close(_epoll_fd);
}

void IOMonitor::_listen() {
    std::unique_lock lock(_run_mutex);
    std::vector<struct epoll_event> events;

    if (_is_running)
        throw std::runtime_error("IOMonitor double run");

    _is_running = true;

    while (_is_running) {
        if (events.size() != _fds.size())
            events.resize(_fds.size());

        int ev_count = ::epoll_wait(_epoll_fd, events.data(), (int) events.size(), -1);
        for (int i = 0; i < ev_count; ++i) {
            std::shared_ptr<IOHandler> handler;

            if (events[i].data.fd == _event_fd) {
                if (events[i].events & EPOLLIN) {
                    lock.unlock();
                    /* Wait until done yielding */
                    const std::lock_guard yield_lock(_yield_mutex);
                    uint64_t event;
                    while (-1 != ::eventfd_read(_event_fd, &event)) { }
                    lock.lock();
                }
            } else {
                try {
                    handler = _fds.at(events[i].data.fd);
                } catch (std::out_of_range& e) {
                    continue;
                }

                lock.unlock();
                try {
                    if (events[i].events & EPOLLIN)
                        handler->read();
                    if (events[i].events & EPOLLHUP)
                        handler->hangup();
                    if (events[i].events & EPOLLERR)
                        handler->error();
                } catch (std::exception& e) {
                    logPrintf(ERROR, "Unhandled I/O handler error: %s", e.what());
                }
                lock.lock();

            }
        }
    }
}

void IOMonitor::_stop() noexcept {
    _is_running = false;
    _yield();
    _io_thread->join();
}

std::unique_lock<std::mutex> IOMonitor::_yield() noexcept {
    /* Prevent listener thread from grabbing lock during yielding */
    std::unique_lock yield_lock(_yield_mutex);

    std::unique_lock run_lock(_run_mutex, std::try_to_lock);
    if (!run_lock.owns_lock()) {
        ::eventfd_write(_event_fd, 1);
        run_lock = std::unique_lock<std::mutex>(_run_mutex);
    }

    return run_lock;
}

void IOMonitor::add(int fd, IOHandler handler) {
    const auto lock = _yield();

    struct epoll_event event{};
    event.events = EPOLLIN | EPOLLHUP | EPOLLERR;
    event.data.fd = fd;

    if (!_fds.contains(fd)) {
        if (::epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &event))
            throw std::system_error(errno, std::generic_category());
        _fds.emplace(fd, std::make_shared<IOHandler>(std::move(handler)));
    } else {
        throw std::runtime_error("duplicate io fd");
    }
}

void IOMonitor::remove(int fd) noexcept {
    const auto lock = _yield();
    ::epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    _fds.erase(fd);
}