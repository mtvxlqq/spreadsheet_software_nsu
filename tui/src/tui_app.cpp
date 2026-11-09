#include "tui/tui_app.hpp"

#include <optional>
#include <string>

#include <ftxui/component/component.hpp>

#include "tui/key_names.hpp"
#include "tui/screen_view.hpp"

namespace ss::tui {

TuiApp::TuiApp(ftxui::App& app, ss::app::Session& session, const ss::core::NumberFormat& format)
    : app_(app), session_(session), format_(format) {
    session_.subscribe(this);
}

TuiApp::~TuiApp() {
    session_.unsubscribe(this);
}

void TuiApp::run() {
    // управление только с клавиатуры
    app_.TrackMouse(false);
    // иначе Ctrl+C закроет программу, и несохранённая таблица пропадёт
    app_.ForceHandleCtrlC(false);
    app_.ForceHandleCtrlZ(false);

    auto screen = ftxui::Renderer([this] { return render(); });
    auto root = ftxui::CatchEvent(screen, [this](const ftxui::Event& event) {
        return handle_event(event);
    });
    app_.Loop(root);

    if (failure_) {
        std::rethrow_exception(failure_);
    }
}

// Зовётся только из главного потока, поэтому флаг без мьютекса. Сам App::Post
// кадр не обновляет, так что после фоновой загрузки экран остался бы старым
// до следующей клавиши. Event::Custom заставляет цикл нарисовать новый кадр.
void TuiApp::on_session_changed() {
    if (redraw_posted_) {
        return;
    }
    redraw_posted_ = true;
    app_.PostEvent(ftxui::Event::Custom);
}

bool TuiApp::handle_event(const ftxui::Event& event) {
    if (event == ftxui::Event::Custom) {
        redraw_posted_ = false;
        return true;
    }
    const std::optional<std::string> key = key_name(event);
    if (!key) {
        return false;
    }
    try {
        session_.handle_key(*key);
    } catch (...) {
        stop_with_failure();
        return true;
    }
    if (session_.quit_requested()) {
        app_.Exit();
    }
    return true;
}

ftxui::Element TuiApp::render() {
    try {
        const ftxui::Dimensions size = ftxui::Terminal::Size();
        sync_size(size);
        return render_screen(session_.view(), format_, size.dimx);
    } catch (...) {
        stop_with_failure();
        return ftxui::text("");
    }
}

// Отдельного события об изменении окна мы не получаем, а перед кадром размер
// известен всегда. Заодно так обрабатывается и самый первый кадр.
void TuiApp::sync_size(const ftxui::Dimensions& size) {
    const ScreenLayout layout = compute_layout(size.dimx, size.dimy);
    if (last_layout_ != layout) {
        last_layout_ = layout;
        session_.resize(layout.visible_rows, layout.visible_cols);
    }
}

// Сквозь цикл FTXUI исключение пускать нельзя, терминал остался бы в сыром
// режиме. Запоминаем, выходим из цикла, а run() бросит его уже после.
void TuiApp::stop_with_failure() {
    failure_ = std::current_exception();
    app_.Exit();
}

}  // namespace ss::tui
