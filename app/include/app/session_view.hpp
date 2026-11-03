#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "app/status.hpp"
#include "app/viewport.hpp"
#include "core/address.hpp"

namespace ss::core {
class Spreadsheet;
}

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
    virtual ss::core::CellAddress cursor() const = 0;
    virtual const Viewport& viewport() const = 0;
    virtual std::string_view mode_name() const = 0;
    virtual const Status& status() const = 0;
    // строка правки или командная строка, смотря какой режим
    virtual const std::string& edit_buffer() const = 0;
    // курсор в edit_buffer(), в байтах
    virtual std::size_t edit_caret() const = 0;
    // в правке это буфер, иначе raw ячейки под курсором
    virtual std::string formula_bar_text() const = 0;
    virtual std::vector<std::string> help_lines() const = 0;
    // есть несохранённые изменения
    virtual bool dirty() const = 0;

protected:
    SessionView() = default;
};

}  // namespace ss::app
