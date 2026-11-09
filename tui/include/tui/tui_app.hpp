#pragma once

#include <exception>
#include <optional>

#include <ftxui/component/app.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include "app/session.hpp"
#include "app/session_observer.hpp"
#include "tui/screen_layout.hpp"

namespace ss::core {
class NumberFormat;
}

namespace ss::tui {

// Цикл событий: клавиши отдаёт сеансу, по его сигналу перерисовывает экран.
class TuiApp final : public ss::app::SessionObserver {
public:
    // app, session и format должны жить дольше TuiApp
    TuiApp(ftxui::App& app, ss::app::Session& session, const ss::core::NumberFormat& format);
    ~TuiApp() override;

    // Работает до выхода из программы. Если внутри случилось то, что сеанс
    // не обработал, исключение вылетит отсюда, когда терминал уже восстановлен.
    void run();

    void on_session_changed() override;

private:
    bool handle_event(const ftxui::Event& event);
    ftxui::Element render();
    void sync_size(const ftxui::Dimensions& size);
    void stop_with_failure();

    ftxui::App& app_;
    ss::app::Session& session_;
    const ss::core::NumberFormat& format_;
    std::optional<ScreenLayout> last_layout_;
    // Event::Custom уже в очереди, второй такой же не нужен
    bool redraw_posted_ = false;
    std::exception_ptr failure_;
};

}  // namespace ss::tui
