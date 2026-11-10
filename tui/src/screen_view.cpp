#include "tui/screen_view.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include <ftxui/screen/color.hpp>
#include <ftxui/screen/string.hpp>

#include "app/cell_presenter.hpp"
#include "app/status.hpp"
#include "app/viewport.hpp"
#include "core/address.hpp"
#include "tui/screen_layout.hpp"
#include "tui/text_fit.hpp"

namespace ss::tui {

namespace {

using ftxui::Element;
using ftxui::Elements;
using ss::app::Align;
using ss::app::CellPresenter;
using ss::app::SessionView;
using ss::core::CellAddress;

// окно шире листа не должно просить ячейки за его краем
int end_row(const ss::app::Viewport& viewport) {
    return std::min(viewport.first_row() + viewport.visible_rows(), ss::core::max_rows);
}

int end_col(const ss::app::Viewport& viewport) {
    return std::min(viewport.first_col() + viewport.visible_cols(), ss::core::max_cols);
}

Element render_caret_line(const std::string& prefix, std::string_view text, std::size_t caret,
                          int width) {
    const CaretWindow window = caret_window(text, caret, width - ftxui::string_width(prefix));
    return ftxui::hbox({
        ftxui::text(prefix),
        ftxui::text(window.before),
        ftxui::text(window.at) | ftxui::inverted,
        ftxui::text(window.after),
    });
}

Element render_formula_bar(const SessionView& view, int width) {
    if (view.uses_command_line()) {
        return render_caret_line(":", view.edit_buffer(), view.edit_caret(), width);
    }
    const std::string prefix = view.cursor().to_string() + ": ";
    if (view.edits_cell()) {
        return render_caret_line(prefix, view.edit_buffer(), view.edit_caret(), width);
    }
    const int room = width - ftxui::string_width(prefix);
    return ftxui::text(prefix + fit_cell(view.formula_bar_text(), room, Align::Left));
}

Element render_column_header(const SessionView& view) {
    Elements cells;
    cells.push_back(ftxui::text(std::string(row_header_width, ' ')));
    const ss::app::Viewport& viewport = view.viewport();
    for (int col = viewport.first_col(); col < end_col(viewport); ++col) {
        cells.push_back(ftxui::text(fit_cell(ss::core::column_name(col), column_width, Align::Left)) |
                        ftxui::bold);
    }
    return ftxui::hbox(std::move(cells));
}

Element render_cell(const SessionView& view, CellAddress address) {
    const ss::app::CellView shown =
        CellPresenter::present(view.document(), address, view.number_formats());
    const Overflow overflow = shown.is_number ? Overflow::Hashes : Overflow::Cut;
    // последняя позиция пустая, чтобы соседние столбцы не слипались
    Element cell =
        ftxui::text(fit_cell(shown.text, column_width - 1, shown.align, overflow) + " ");
    return address == view.cursor() ? std::move(cell) | ftxui::inverted : cell;
}

Element render_row(const SessionView& view, int row) {
    Elements cells;
    const std::string number =
        fit_cell(std::to_string(row + 1), row_header_width - 1, Align::Right) + " ";
    cells.push_back(ftxui::text(number) | ftxui::bold);
    const ss::app::Viewport& viewport = view.viewport();
    for (int col = viewport.first_col(); col < end_col(viewport); ++col) {
        cells.push_back(render_cell(view, {.row = row, .col = col}));
    }
    return ftxui::hbox(std::move(cells));
}

std::string document_name(const SessionView& view) {
    std::string name = view.current_path() ? view.current_path()->filename().string() : "без имени";
    if (view.dirty()) {
        name += '*';
    }
    return name;
}

Element render_status_bar(const SessionView& view, int width) {
    const ss::app::Status& status = view.status();
    // следующая клавиша стирает сообщение, а загрузка ещё идёт
    const std::string message = status.text.empty() && view.busy() ? "загрузка..." : status.text;
    const std::string mode(view.mode_name());
    const std::string name = "  " + document_name(view) + "  ";
    // иначе длинное сообщение сожмёт соседей, и режим станет нечитаемым
    const int room = width - ftxui::string_width(mode) - ftxui::string_width(name);
    Element message_element = ftxui::text(fit_cell(message, room, Align::Left));
    if (status.kind == ss::app::Status::Kind::Error) {
        message_element = std::move(message_element) | ftxui::color(ftxui::Color::Red);
    }
    return ftxui::hbox({
        ftxui::text(mode) | ftxui::bold,
        ftxui::text(name),
        std::move(message_element),
    });
}

Element render_help(const SessionView& view) {
    Elements lines;
    lines.push_back(ftxui::text("Справка, любая клавиша вернёт к таблице") | ftxui::bold);
    for (const std::string& line : view.help_lines()) {
        lines.push_back(ftxui::text(line));
    }
    return ftxui::vbox(std::move(lines));
}

Element render_table(const SessionView& view, int width) {
    Elements lines;
    lines.push_back(render_formula_bar(view, width));
    lines.push_back(render_column_header(view));
    const ss::app::Viewport& viewport = view.viewport();
    for (int row = viewport.first_row(); row < end_row(viewport); ++row) {
        lines.push_back(render_row(view, row));
    }
    return ftxui::vbox(std::move(lines));
}

}  // namespace

Element render_screen(const SessionView& view, int width) {
    Element body = view.shows_help() ? render_help(view) : render_table(view, width);
    return ftxui::vbox({
        std::move(body) | ftxui::flex,
        render_status_bar(view, width),
    });
}

}  // namespace ss::tui
