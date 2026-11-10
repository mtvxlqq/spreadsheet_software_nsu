#include "actions/clipboard_actions.hpp"

#include <memory>
#include <string>
#include <utility>

#include "actions/no_args.hpp"
#include "app/clipboard.hpp"
#include "app/errors.hpp"
#include "app/modes/select_mode.hpp"
#include "app/paste_command.hpp"
#include "app/session.hpp"
#include "core/spreadsheet.hpp"

namespace ss::app {

std::string_view StartSelectionAction::name() const {
    return "start-selection";
}

std::string_view StartSelectionAction::description() const {
    return "выделить диапазон: дальше стрелки, esc снимает";
}

void StartSelectionAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name(), args);
    session.start_selection();
    session.set_mode(std::make_unique<SelectMode>());
}

std::string_view CopyAction::name() const {
    return "copy";
}

std::string_view CopyAction::description() const {
    return "копировать ячейку или выделенное";
}

void CopyAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name(), args);
    const ss::core::Spreadsheet& document = session.document();
    Clipboard clipboard{.source = session.target_range(), .items = {}};
    // Обходим заполненные ячейки листа, а не весь диапазон: он бывает огромным.
    document.for_each_cell([&](ss::core::CellAddress address) {
        if (clipboard.source.contains(address)) {
            clipboard.items.push_back({.address = address, .content = document.content(address)});
        }
    });
    session.set_status("скопировано " + clipboard.source.to_string());
    session.set_clipboard(std::move(clipboard));
}

std::string_view PasteAction::name() const {
    return "paste";
}

std::string_view PasteAction::description() const {
    return "вставить скопированное, ссылки без $ сдвигаются";
}

void PasteAction::execute(Session& session, std::span<const std::string> args) const {
    expect_no_args(name(), args);
    if (!session.clipboard()) {
        throw CommandError("вставлять нечего, сначала скопируйте ячейки");
    }
    // у выделения берём левый верхний угол
    session.apply_edit(std::make_unique<PasteCommand>(
        *session.clipboard(), session.target_range().from(), session.functions()));
}

}  // namespace ss::app
