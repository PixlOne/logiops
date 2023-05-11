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
#ifndef LOGID_TASK_H
#define LOGID_TASK_H

#include <util/ExceptionHandler.h>
#include <functional>
#include <memory>
#include <future>

namespace logid {
    struct task {
        std::function<void()> function;
        std::chrono::time_point<std::chrono::system_clock> time;
    };

    void init_workers(int worker_count);

    void run_task(std::function<void()> function);
    void run_task_after(std::function<void()> function, std::chrono::milliseconds delay);
    void run_task(task t);
}

#endif //LOGID_TASK_H
