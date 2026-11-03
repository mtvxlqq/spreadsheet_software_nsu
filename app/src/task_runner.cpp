#include "app/task_runner.hpp"

#include <exception>
#include <utility>

#include "app/main_thread_dispatcher.hpp"

namespace ss::app {

namespace {

// это выполняется в фоновом потоке
void run(MainThreadDispatcher& dispatcher, const TaskRunner::Work& work, std::stop_token stop,
         std::atomic<bool>& done) {
    TaskRunner::Continuation next;
    try {
        next = work(stop);
    } catch (...) {
        // исключение не может покинуть поток (будет std::terminate),
        // поэтому переносим его в главный поток
        next = [error = std::current_exception()] { std::rethrow_exception(error); };
    }
    if (next && !stop.stop_requested()) {
        dispatcher.post(std::move(next));
    }
    done = true;
}

}  // namespace

TaskRunner::TaskRunner(MainThreadDispatcher& dispatcher) : dispatcher_(dispatcher) {}

TaskRunner::~TaskRunner() {
    // сначала просим остановиться всех, потом ждём, а не по одному
    for (Task& task : tasks_) {
        task.thread.request_stop();
    }
    // join делает деструктор jthread
}

void TaskRunner::submit(Work work) {
    remove_finished();
    auto done = std::make_shared<std::atomic<bool>>(false);
    std::jthread thread([&dispatcher = dispatcher_, work = std::move(work), done](
                            std::stop_token stop) { run(dispatcher, work, stop, *done); });
    tasks_.push_back({.done = std::move(done), .thread = std::move(thread)});
}

void TaskRunner::remove_finished() {
    // поток уже закончил, так что join в деструкторе jthread не ждёт
    tasks_.remove_if([](const Task& task) { return task.done->load(); });
}

}  // namespace ss::app
