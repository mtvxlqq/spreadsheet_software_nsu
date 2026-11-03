#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <vector>

#include "app/main_thread_dispatcher.hpp"

namespace ss::test {

// Для тестов: задачи копятся в очереди, а выполняет их сам тест через
// drain(), то есть в потоке теста. Так видно, что до drain() главный
// поток ничего не получил.
class ManualDispatcher final : public ss::app::MainThreadDispatcher {
public:
    void post(std::function<void()> task) override {
        {
            const std::lock_guard lock(mutex_);
            queue_.push_back(std::move(task));
        }
        posted_.notify_all();
    }

    // выполнить всё, что уже пришло, и вернуть сколько
    std::size_t drain() {
        std::vector<std::function<void()>> tasks;
        {
            const std::lock_guard lock(mutex_);
            tasks.swap(queue_);
        }
        for (const auto& task : tasks) {
            task();
        }
        return tasks.size();
    }

    // ждать, пока в очереди что-нибудь появится; false, если не дождались
    bool wait_for_task(std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
        std::unique_lock lock(mutex_);
        return posted_.wait_for(lock, timeout, [this] { return !queue_.empty(); });
    }

    std::size_t pending() const {
        const std::lock_guard lock(mutex_);
        return queue_.size();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable posted_;
    std::vector<std::function<void()>> queue_;
};

}  // namespace ss::test
