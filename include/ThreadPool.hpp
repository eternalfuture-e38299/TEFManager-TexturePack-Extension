/*******************************************************************************
 * texturepack_extension - ThreadPool
 * Copyright (C) 2026 eternalfuture-e38299
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Author: eternalfuture-e38299
 * GitHub: https://github.com/eternalfuture-e38299
 * Created: 2026/9/29
 *******************************************************************************/

#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

class ThreadPool final {
public:
    explicit ThreadPool(const size_t threadCount) {
        _workers.reserve(threadCount);
        for (size_t i = 0; i < threadCount; ++i) {
            _workers.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(_mutex);
                        _cv.wait(lock, [this] { return _stop || !_tasks.empty(); });
                        if (_stop && _tasks.empty()) return;
                        task = std::move(_tasks.front());
                        _tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard lock(_mutex);
            _stop = true;
        }
        _cv.notify_all();
        for (auto &t: _workers) {
            if (t.joinable()) t.join();
        }
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    template<typename F, typename... Args>
    auto Enqueue(F &&f, Args &&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        using R = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<R()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<R> fut = task->get_future();
        {
            std::lock_guard lock(_mutex);
            if (_stop) throw std::runtime_error("ThreadPool stopped");
            _tasks.emplace([task] { (*task)(); });
        }
        _cv.notify_one();
        return fut;
    }

    [[nodiscard]] size_t ThreadCount() const { return _workers.size(); }

private:
    std::vector<std::thread> _workers;
    std::queue<std::function<void()>> _tasks;
    std::mutex _mutex;
    std::condition_variable _cv;
    bool _stop = false;
};