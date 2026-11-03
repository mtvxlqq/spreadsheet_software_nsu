#include "app/session.hpp"

#include <algorithm>
#include <utility>

#include "app/edit_command.hpp"
#include "app/modes/help_mode.hpp"
#include "app/modes/normal_mode.hpp"
#include "app/session_observer.hpp"
#include "core/errors.hpp"

namespace ss::app {

Session::Session(const ss::core::FunctionProvider& functions, const ActionRegistry& actions,
                 Keymap keymap)
    : actions_(actions),
      keymap_(std::move(keymap)),
      document_(std::make_unique<ss::core::Spreadsheet>(functions)),
      mode_(std::make_unique<NormalMode>()) {}

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
    if (!cell.is_valid()) {
        throw ss::core::LimitError("ячейка " + cell.to_string() + " за пределами листа");
    }
    cursor_ = cell;
    viewport_.ensure_visible(cursor_);
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
    dirty_ = true;
    // на втором этапе тут команда положится в стек отмены
}

void Session::set_status(std::string text, Status::Kind kind) {
    status_ = {.text = std::move(text), .kind = kind};
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

ss::core::CellAddress Session::cursor() const {
    return cursor_;
}

const Viewport& Session::viewport() const {
    return viewport_;
}

std::string_view Session::mode_name() const {
    return mode_->name();
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

}  // namespace ss::app
