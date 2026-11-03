#pragma once

#include <functional>

namespace ss::app {

// Как передать задачу в главный поток. В программе это сделает TUI
// (у FTXUI есть screen.Post), в тестах обычная очередь.
class MainThreadDispatcher {
public:
    virtual ~MainThreadDispatcher() = default;
    MainThreadDispatcher(const MainThreadDispatcher&) = delete;
    MainThreadDispatcher& operator=(const MainThreadDispatcher&) = delete;
    MainThreadDispatcher(MainThreadDispatcher&&) = delete;
    MainThreadDispatcher& operator=(MainThreadDispatcher&&) = delete;

    // Зовут из фоновых потоков, так что реализация должна быть
    // потокобезопасной. task потом выполнится в главном потоке.
    virtual void post(std::function<void()> task) = 0;

protected:
    MainThreadDispatcher() = default;
};

}  // namespace ss::app
