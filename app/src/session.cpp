#include "app/session.hpp"

#include <algorithm>
#include <utility>

#include "app/edit_command.hpp"
#include "app/modes/help_mode.hpp"
#include "app/modes/normal_mode.hpp"
#include "app/session_observer.hpp"
#include "core/errors.hpp"

namespace ss::app {

Session::Session(const ss::core::FunctionProvider& functions,
                 const ss::core::FormatRegistry& formats,
                 const ss::core::NumberFormatRegistry& number_formats,
                 const ActionRegistry& actions, Keymap keymap, MainThreadDispatcher& dispatcher)
    : functions_(functions),
      formats_(formats),
      number_formats_(number_formats),
      actions_(actions),
      keymap_(std::move(keymap)),
      document_(std::make_unique<ss::core::Spreadsheet>(functions)),
      mode_(std::make_unique<NormalMode>()),
      tasks_(dispatcher) {}

Session::~Session() = default;

void Session::handle_key(std::string_view key) {
    status_ = {};
    handling_key_ = true;
    try {
        mode_->handle_key(*this, key);
    } catch (const ss::core::Error& error) {
        // клавиша не сработала, значит и режим остаётся прежним
        pending_mode_.reset();
        set_status(error.what(), Status::Kind::Error);
    } catch (...) {
        handling_key_ = false;
        pending_mode_.reset();
        throw;
    }
    handling_key_ = false;
    // старый режим уже вернул управление, теперь его можно удалить
    if (pending_mode_) {
        mode_ = std::move(pending_mode_);
    }
    notify();
}

void Session::resize(int visible_rows, int visible_cols) {
    viewport_.resize(visible_rows, visible_cols);
    viewport_.ensure_visible(cursor_);
    notify();
}

void Session::move_cursor(int d_row, int d_col) {
    // считаем в long long, чтобы большой шаг не переполнил int
    const auto clamp = [](int value, int delta, int limit) {
        const long long moved = static_cast<long long>(value) + delta;
        return static_cast<int>(std::clamp<long long>(moved, 0, limit - 1));
    };
    set_cursor({.row = clamp(cursor_.row, d_row, ss::core::max_rows),
                .col = clamp(cursor_.col, d_col, ss::core::max_cols)});
}

void Session::set_cursor(ss::core::CellAddress cell) {
    cell.check_valid();
    cursor_ = cell;
    viewport_.ensure_visible(cursor_);
}

ss::core::Range Session::target_range() const {
    return {cursor_, cursor_};
}

void Session::set_mode(std::unique_ptr<Mode> mode) {
    if (handling_key_) {
        pending_mode_ = std::move(mode);
    } else {
        mode_ = std::move(mode);
    }
}

void Session::apply_edit(std::unique_ptr<EditCommand> command) {
    command->apply(*document_);
    history_.push(std::move(command));
    dirty_ = true;
}

// После отмены документ всё равно считаем изменённым: сравнивать его
// с сохранённым дорого, а лишний вопрос при выходе не страшен.
bool Session::undo() {
    if (!history_.undo(*document_)) {
        return false;
    }
    dirty_ = true;
    return true;
}

bool Session::redo() {
    if (!history_.redo(*document_)) {
        return false;
    }
    dirty_ = true;
    return true;
}

void Session::set_status(std::string text, Status::Kind kind) {
    status_ = {.text = std::move(text), .kind = kind};
}

void Session::replace_document(std::unique_ptr<ss::core::Spreadsheet> document,
                               std::filesystem::path path) {
    set_document(std::move(document));
    current_path_ = std::move(path);
    dirty_ = false;
}

void Session::import_document(std::unique_ptr<ss::core::Spreadsheet> document) {
    set_document(std::move(document));
    current_path_.reset();
    dirty_ = true;
}

void Session::mark_saved(std::filesystem::path path) {
    current_path_ = std::move(path);
    dirty_ = false;
}

void Session::request_quit() {
    quit_requested_ = true;
}

void Session::run_in_background(BackgroundWork work) {
    ++background_tasks_;
    const std::weak_ptr<bool> alive = alive_;
    // эта лямбда выполняется в фоне: this тут только запоминаем
    tasks_.submit([this, alive, work = std::move(work)](std::stop_token stop) {
        std::function<void(Session&)> next;
        std::exception_ptr error;
        try {
            next = work(stop);
        } catch (...) {
            error = std::current_exception();
        }
        // а эта уже в главном потоке, там же, где удаляется сеанс,
        // поэтому проверка alive и обращение к this не пересекутся
        return [this, alive, next = std::move(next), error] {
            if (!alive.expired()) {
                finish_background(next, error);
            }
        };
    });
}

void Session::finish_background(const std::function<void(Session&)>& next,
                                const std::exception_ptr& error) {
    --background_tasks_;
    try {
        if (error) {
            std::rethrow_exception(error);
        }
        if (next) {
            next(*this);
        }
    } catch (const std::exception& exception) {
        // в фоне может прилететь и не наша ошибка, например от файловой
        // системы. Это всё равно просто неудачная загрузка.
        set_status(exception.what(), Status::Kind::Error);
    }
    notify();
}

void Session::set_document(std::unique_ptr<ss::core::Spreadsheet> document) {
    document_ = std::move(document);
    // команды из стека помнят ячейки старого документа
    history_.clear();
    set_cursor({.row = 0, .col = 0});
}

void Session::subscribe(SessionObserver* observer) {
    if (std::ranges::find(observers_, observer) == observers_.end()) {
        observers_.push_back(observer);
    }
}

void Session::unsubscribe(SessionObserver* observer) {
    std::erase(observers_, observer);
}

void Session::notify() {
    // копия, потому что наблюдатель может отписаться прямо в уведомлении
    const std::vector<SessionObserver*> observers = observers_;
    for (SessionObserver* observer : observers) {
        if (std::ranges::find(observers_, observer) != observers_.end()) {
            observer->on_session_changed();
        }
    }
}

const ss::core::Spreadsheet& Session::document() const {
    return *document_;
}

const ss::core::NumberFormatRegistry& Session::number_formats() const {
    return number_formats_;
}

ss::core::CellAddress Session::cursor() const {
    return cursor_;
}

const Viewport& Session::viewport() const {
    return viewport_;
}

std::string_view Session::mode_name() const {
    return mode_->name();
}

bool Session::edits_cell() const {
    return mode_->edits_cell();
}

bool Session::shows_help() const {
    return mode_->shows_help();
}

bool Session::uses_command_line() const {
    return mode_->uses_command_line();
}

const Status& Session::status() const {
    return status_;
}

const std::string& Session::edit_buffer() const {
    return edit_line_.text();
}

std::size_t Session::edit_caret() const {
    return edit_line_.caret();
}

std::string Session::formula_bar_text() const {
    if (mode_->edits_cell()) {
        return edit_line_.text();
    }
    return document_->raw(cursor_);
}

std::vector<std::string> Session::help_lines() const {
    return HelpMode::lines(actions_, keymap_);
}

bool Session::dirty() const {
    return dirty_;
}

const std::optional<std::filesystem::path>& Session::current_path() const {
    return current_path_;
}

bool Session::quit_requested() const {
    return quit_requested_;
}

bool Session::busy() const {
    return background_tasks_ > 0;
}

}  // namespace ss::app
