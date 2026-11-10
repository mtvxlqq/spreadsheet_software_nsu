#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/status.hpp"
#include "app/viewport.hpp"
#include "core/address.hpp"

namespace ss::core {
class NumberFormatRegistry;
class Spreadsheet;
}  // namespace ss::core

namespace ss::app {

// Всё, что нужно TUI для рисования, только на чтение. Менять сеанс
// TUI может лишь через handle_key и resize.
class SessionView {
public:
    virtual ~SessionView() = default;
    SessionView(const SessionView&) = delete;
    SessionView& operator=(const SessionView&) = delete;
    SessionView(SessionView&&) = delete;
    SessionView& operator=(SessionView&&) = delete;

    virtual const ss::core::Spreadsheet& document() const = 0;
    // по ним CellPresenter показывает числа
    virtual const ss::core::NumberFormatRegistry& number_formats() const = 0;
    virtual ss::core::CellAddress cursor() const = 0;
    // выделенный диапазон, его надо подсветить; пусто, если выделения нет
    virtual std::optional<ss::core::Range> selection() const = 0;
    virtual const Viewport& viewport() const = 0;
    virtual std::string_view mode_name() const = 0;
    // в строке формулы правка, и там рисуется каретка
    virtual bool edits_cell() const = 0;
    // вместо таблицы рисуется справка
    virtual bool shows_help() const = 0;
    // на месте строки формулы рисуется командная строка
    virtual bool uses_command_line() const = 0;
    virtual const Status& status() const = 0;
    // строка правки или командная строка, смотря какой режим
    virtual const std::string& edit_buffer() const = 0;
    // курсор в edit_buffer(), в байтах
    virtual std::size_t edit_caret() const = 0;
    // в правке это буфер, иначе raw ячейки под курсором
    virtual std::string formula_bar_text() const = 0;
    virtual std::vector<std::string> help_lines() const = 0;
    // сколько первых строк справки пропустить: она длиннее экрана
    virtual int help_scroll() const = 0;
    // есть несохранённые изменения
    virtual bool dirty() const = 0;
    // откуда открыт или куда последний раз сохранён документ
    virtual const std::optional<std::filesystem::path>& current_path() const = 0;
    // TUI проверяет после каждой клавиши и выходит из цикла
    virtual bool quit_requested() const = 0;
    // идёт фоновая загрузка, можно показать "загрузка..."
    virtual bool busy() const = 0;

protected:
    SessionView() = default;
};

}  // namespace ss::app
