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
#ifndef LOGID_WORKQUEUE_H
#define LOGID_WORKQUEUE_H

#include "worker_thread.h"
#include "thread.h"

namespace logid
{
    class workqueue
    {
    public:
        explicit workqueue(std::size_t thread_count);
        ~workqueue();

        void queue(std::shared_ptr<task> t);

        void busyUpdate();

        void stop();

        void setThreadCount(std::size_t count);
        std::size_t threadCount() const;
    private:
        void _run();

        void _exception_handler(std::exception& e);
        std::unique_ptr<thread> _manager_thread;

        mutex_queue<std::shared_ptr<task>> _queue;
        std::condition_variable _queue_cv;
        std::condition_variable _busy_cv;
        std::mutex _run_lock;
        std::atomic<bool> _continue_run;

        std::vector<std::unique_ptr<worker_thread>> _workers;
        std::size_t _worker_count;
    };

    extern std::unique_ptr<workqueue> global_workqueue;
}

#endif //LOGID_WORKQUEUE_H
