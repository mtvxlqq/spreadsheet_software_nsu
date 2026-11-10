#pragma once

#include <cstddef>
#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/clipboard.hpp"
#include "app/keymap.hpp"
#include "app/line_editor.hpp"
#include "app/mode.hpp"
#include "app/session_view.hpp"
#include "app/status.hpp"
#include "app/task_runner.hpp"
#include "app/undo_stack.hpp"
#include "app/viewport.hpp"
#include "core/address.hpp"
#include "core/spreadsheet.hpp"

namespace ss::core {
class FormatRegistry;
class FunctionProvider;
class NumberFormatRegistry;
}  // namespace ss::core

namespace ss::app {

class ActionRegistry;
class EditCommand;
class MainThreadDispatcher;
class SessionObserver;

// Текущая работа с программой: документ, курсор, режим, строка правки,
// статус. Клавиши передаются текущему режиму, а режимы и действия
// меняют сеанс через методы ниже.
class Session final : public SessionView {
public:
    // Выполняется в фоне и не трогает сеанс. Возвращает то, что потом
    // сделать с сеансом в главном потоке.
    using BackgroundWork = std::function<std::function<void(Session&)>(std::stop_token)>;

    // реестры и dispatcher должны жить дольше сеанса
    Session(const ss::core::FunctionProvider& functions, const ss::core::FormatRegistry& formats,
            const ss::core::NumberFormatRegistry& number_formats, const ActionRegistry& actions,
            Keymap keymap, MainThreadDispatcher& dispatcher);
    ~Session() override;

    // Вход от TUI. Ошибки (ss::core::Error) не вылетают наружу,
    // а показываются в статусе. После каждого вызова наблюдатели
    // получают on_session_changed.
    void handle_key(std::string_view key);
    // grid_width это ширина сетки в позициях экрана, без номеров строк
    void resize(int visible_rows, int grid_width);

    // курсор не уходит за края листа, экран прокручивается за ним
    void move_cursor(int d_row, int d_col);
    void set_cursor(ss::core::CellAddress cell);
    // Выделение от ячейки под курсором, дальше оно тянется за курсором.
    // Снимать его не нужно: оно пропадает само, как только режим перестаёт
    // его держать (Mode::keeps_selection).
    void start_selection();
    // Ячейки, с которыми работает команда: выделенный диапазон, а если
    // выделения нет, ячейка под курсором.
    ss::core::Range target_range() const;
    void set_clipboard(Clipboard clipboard);
    // пусто, пока ничего не копировали
    const std::optional<Clipboard>& clipboard() const;

    // Внутри handle_key режим сменится только после выхода из старого
    // режима, иначе мы бы удалили объект, чей метод сейчас выполняется.
    void set_mode(std::unique_ptr<Mode> mode);
    // всё, что меняет документ, проходит здесь; если бросит, ничего не изменилось
    void apply_edit(std::unique_ptr<EditCommand> command);
    // false, если отменять или повторять нечего
    bool undo();
    bool redo();
    void set_status(std::string text, Status::Kind kind = Status::Kind::Info);
    // с какой строки показывать справку; лишнее обрезается до последней страницы
    void set_help_scroll(int line);

    // открыли файл: он становится текущим, несохранённых изменений нет
    void replace_document(std::unique_ptr<ss::core::Spreadsheet> document,
                          std::filesystem::path path);
    // импорт: файла у документа нет, и его ещё надо сохранить
    void import_document(std::unique_ptr<ss::core::Spreadsheet> document);
    // документ записан в path
    void mark_saved(std::filesystem::path path);
    void request_quit();

    // Если work бросит, ошибка попадёт в статус. Если сеанс к тому
    // времени уже удалён, продолжение просто ничего не сделает.
    void run_in_background(BackgroundWork work);

    LineEditor& edit_line() {
        return edit_line_;
    }

    const Keymap& keymap() const {
        return keymap_;
    }

    const ActionRegistry& actions() const {
        return actions_;
    }

    const ss::core::FunctionProvider& functions() const {
        return functions_;
    }

    const ss::core::FormatRegistry& formats() const {
        return formats_;
    }

    // подписчик обязан отписаться до своего уничтожения
    void subscribe(SessionObserver* observer);
    void unsubscribe(SessionObserver* observer);

    const SessionView& view() const {
        return *this;
    }

    const ss::core::Spreadsheet& document() const override;
    const ss::core::NumberFormatRegistry& number_formats() const override;
    ss::core::CellAddress cursor() const override;
    std::optional<ss::core::Range> selection() const override;
    const Viewport& viewport() const override;
    std::string_view mode_name() const override;
    bool edits_cell() const override;
    bool shows_help() const override;
    bool uses_command_line() const override;
    const Status& status() const override;
    const std::string& edit_buffer() const override;
    std::size_t edit_caret() const override;
    std::string formula_bar_text() const override;
    std::vector<std::string> help_lines() const override;
    int help_scroll() const override;
    bool dirty() const override;
    const std::optional<std::filesystem::path>& current_path() const override;
    bool quit_requested() const override;
    bool busy() const override;

private:
    void notify();
    void finish_background(const std::function<void(Session&)>& next,
                           const std::exception_ptr& error);
    void set_document(std::unique_ptr<ss::core::Spreadsheet> document);
    // после правки ширина столбцов могла измениться
    void keep_cursor_visible();
    void drop_unused_selection();

    const ss::core::FunctionProvider& functions_;
    const ss::core::FormatRegistry& formats_;
    const ss::core::NumberFormatRegistry& number_formats_;
    const ActionRegistry& actions_;
    Keymap keymap_;
    std::unique_ptr<ss::core::Spreadsheet> document_;
    ss::core::CellAddress cursor_;
    // угол выделения, второй угол это курсор; пусто, если выделения нет
    std::optional<ss::core::CellAddress> selection_anchor_;
    std::optional<Clipboard> clipboard_;
    Viewport viewport_;
    LineEditor edit_line_;
    Status status_;
    int help_scroll_ = 0;
    UndoStack history_;
    std::unique_ptr<Mode> mode_;
    // режим, на который переключимся после текущей клавиши
    std::unique_ptr<Mode> pending_mode_;
    bool handling_key_ = false;
    bool dirty_ = false;
    std::optional<std::filesystem::path> current_path_;
    bool quit_requested_ = false;
    int background_tasks_ = 0;
    std::vector<SessionObserver*> observers_;
    // Фоновые продолжения держат на него weak_ptr: истёк, значит сеанса уже нет.
    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
    // Последним полем, чтобы удалиться первым: деструктор дождётся фоновых
    // потоков, пока остальные поля сеанса ещё живы.
    TaskRunner tasks_;
};

}  // namespace ss::app
