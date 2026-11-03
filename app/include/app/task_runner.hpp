#pragma once

#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <stop_token>
#include <thread>

namespace ss::app {

class MainThreadDispatcher;

// Запускает работу в отдельном потоке, а её продолжение отправляет
// в главный поток. Состояние программы меняет только продолжение,
// поэтому мьютексы вокруг документа не нужны.
class TaskRunner {
public:
    using Continuation = std::function<void()>;
    // выполняется в фоне и возвращает то, что надо сделать в главном потоке
    using Work = std::function<Continuation(std::stop_token)>;

    // dispatcher должен жить дольше TaskRunner
    explicit TaskRunner(MainThreadDispatcher& dispatcher);
    // просит все потоки остановиться и дожидается их
    ~TaskRunner();

    TaskRunner(const TaskRunner&) = delete;
    TaskRunner& operator=(const TaskRunner&) = delete;
    TaskRunner(TaskRunner&&) = delete;
    TaskRunner& operator=(TaskRunner&&) = delete;

    // Если work бросит, в главный поток уйдёт продолжение, которое
    // бросит это же исключение. После остановки продолжения не шлются.
    void submit(Work work);

private:
    struct Task {
        std::shared_ptr<std::atomic<bool>> done;
        std::jthread thread;
    };

    // забираем потоки, которые уже закончили
    void remove_finished();

    MainThreadDispatcher& dispatcher_;
    std::list<Task> tasks_;
};

}  // namespace ss::app
