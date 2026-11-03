#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "app/keymap.hpp"
#include "app/line_editor.hpp"
#include "app/mode.hpp"
#include "app/session_view.hpp"
#include "app/status.hpp"
#include "app/viewport.hpp"
#include "core/address.hpp"
#include "core/spreadsheet.hpp"

namespace ss::core {
class FunctionProvider;
}

namespace ss::app {

class ActionRegistry;
class EditCommand;
class SessionObserver;

// Текущая работа с программой: документ, курсор, режим, строка правки,
// статус. Клавиши передаются текущему режиму, а режимы и действия
// меняют сеанс через методы ниже.
class Session final : public SessionView {
public:
    // functions и actions должны жить дольше сеанса
    Session(const ss::core::FunctionProvider& functions, const ActionRegistry& actions,
            Keymap keymap);
    ~Session() override;

    // Вход от TUI. Ошибки (ss::core::Error) не вылетают наружу,
    // а показываются в статусе. После каждого вызова наблюдатели
    // получают on_session_changed.
    void handle_key(std::string_view key);
    void resize(int visible_rows, int visible_cols);

    // курсор не уходит за края листа, экран прокручивается за ним
    void move_cursor(int d_row, int d_col);
    void set_cursor(ss::core::CellAddress cell);

    // Внутри handle_key режим сменится только после выхода из старого
    // режима, иначе мы бы удалили объект, чей метод сейчас выполняется.
    void set_mode(std::unique_ptr<Mode> mode);
    // всё, что меняет документ, проходит здесь; если бросит, ничего не изменилось
    void apply_edit(std::unique_ptr<EditCommand> command);
    void set_status(std::string text, Status::Kind kind = Status::Kind::Info);

    LineEditor& edit_line() {
        return edit_line_;
    }

    const Keymap& keymap() const {
        return keymap_;
    }

    const ActionRegistry& actions() const {
        return actions_;
    }

    // подписчик обязан отписаться до своего уничтожения
    void subscribe(SessionObserver* observer);
    void unsubscribe(SessionObserver* observer);

    const SessionView& view() const {
        return *this;
    }

    const ss::core::Spreadsheet& document() const override;
    ss::core::CellAddress cursor() const override;
    const Viewport& viewport() const override;
    std::string_view mode_name() const override;
    const Status& status() const override;
    const std::string& edit_buffer() const override;
    std::size_t edit_caret() const override;
    std::string formula_bar_text() const override;
    std::vector<std::string> help_lines() const override;
    bool dirty() const override;

private:
    void notify();

    const ActionRegistry& actions_;
    Keymap keymap_;
    std::unique_ptr<ss::core::Spreadsheet> document_;
    ss::core::CellAddress cursor_;
    Viewport viewport_;
    LineEditor edit_line_;
    Status status_;
    std::unique_ptr<Mode> mode_;
    // режим, на который переключимся после текущей клавиши
    std::unique_ptr<Mode> pending_mode_;
    bool handling_key_ = false;
    bool dirty_ = false;
    std::vector<SessionObserver*> observers_;
};

}  // namespace ss::app
