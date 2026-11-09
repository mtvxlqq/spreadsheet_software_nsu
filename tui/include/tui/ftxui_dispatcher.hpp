#pragma once

#include <functional>
#include <utility>

#include <ftxui/component/app.hpp>

#include "app/main_thread_dispatcher.hpp"

namespace ss::tui {

// Задачи из фоновых потоков выполняет цикл событий FTXUI в главном потоке.
class FtxuiDispatcher final : public ss::app::MainThreadDispatcher {
public:
    // app должен жить дольше диспетчера
    explicit FtxuiDispatcher(ftxui::App& app) : app_(app) {}

    void post(std::function<void()> task) override {
        app_.Post(std::move(task));
    }

private:
    ftxui::App& app_;
};

}  // namespace ss::tui
