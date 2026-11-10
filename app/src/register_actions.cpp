#include "app/register_actions.hpp"

#include <memory>
#include <string>

#include "actions/clipboard_actions.hpp"
#include "actions/edit_actions.hpp"
#include "actions/file_actions.hpp"
#include "actions/mode_actions.hpp"
#include "actions/navigation_actions.hpp"
#include "actions/quit_actions.hpp"
#include "actions/structure_actions.hpp"
#include "actions/style_actions.hpp"
#include "actions/width_actions.hpp"
#include "app/action_registry.hpp"
#include "core/ref_mapping.hpp"

namespace ss::app {

namespace {

void add_move(ActionRegistry& registry, const std::string& name, const std::string& description,
              int d_row, int d_col) {
    registry.add(name, [=] { return std::make_unique<MoveAction>(name, description, d_row, d_col); });
}

void add_page(ActionRegistry& registry, const std::string& name, const std::string& description,
              int direction) {
    registry.add(name, [=] { return std::make_unique<PageAction>(name, description, direction); });
}

void add_structure(ActionRegistry& registry, const std::string& name,
                   const std::string& description, ss::core::StructureEdit::Kind kind,
                   ss::core::Axis axis) {
    registry.add(
        name, [=] { return std::make_unique<StructureAction>(name, description, kind, axis); });
}

void add_width_step(ActionRegistry& registry, const std::string& name,
                    const std::string& description, int delta) {
    registry.add(
        name, [=] { return std::make_unique<ColumnWidthStepAction>(name, description, delta); });
}

}  // namespace

void register_actions(ActionRegistry& registry) {
    add_move(registry, "move-up", "курсор вверх", -1, 0);
    add_move(registry, "move-down", "курсор вниз", 1, 0);
    add_move(registry, "move-left", "курсор влево", 0, -1);
    add_move(registry, "move-right", "курсор вправо", 0, 1);
    add_page(registry, "page-up", "на экран вверх", -1);
    add_page(registry, "page-down", "на экран вниз", 1);
    registry.add("go-home", [] { return std::make_unique<GoHomeAction>(); });
    registry.add("go-top", [] { return std::make_unique<GoTopAction>(); });
    registry.add("start-edit", [] { return std::make_unique<StartEditAction>(); });
    registry.add("clear-cell", [] { return std::make_unique<ClearCellAction>(); });
    registry.add("undo", [] { return std::make_unique<UndoAction>(); });
    registry.add("redo", [] { return std::make_unique<RedoAction>(); });
    registry.add("start-selection", [] { return std::make_unique<StartSelectionAction>(); });
    registry.add("copy", [] { return std::make_unique<CopyAction>(); });
    registry.add("paste", [] { return std::make_unique<PasteAction>(); });
    using ss::core::Axis;
    using Kind = ss::core::StructureEdit::Kind;
    add_structure(registry, "insert-row", "вставить строки выше (:ir [сколько])", Kind::Insert,
                  Axis::Row);
    add_structure(registry, "delete-row", "удалить строки (:dr [сколько])", Kind::Delete,
                  Axis::Row);
    add_structure(registry, "insert-col", "вставить столбцы левее (:ic [сколько])", Kind::Insert,
                  Axis::Column);
    add_structure(registry, "delete-col", "удалить столбцы (:dc [сколько])", Kind::Delete,
                  Axis::Column);
    registry.add("format", [] { return std::make_unique<FormatAction>(); });
    registry.add("align", [] { return std::make_unique<AlignAction>(); });
    add_width_step(registry, "column-wider", "столбец шире", 1);
    add_width_step(registry, "column-narrower", "столбец уже", -1);
    registry.add("width", [] { return std::make_unique<WidthAction>(); });
    registry.add("command-mode", [] { return std::make_unique<CommandModeAction>(); });
    registry.add("show-help", [] { return std::make_unique<ShowHelpAction>(); });
    registry.add("save", [] { return std::make_unique<SaveAction>(); });
    registry.add("open", [] { return std::make_unique<OpenAction>(); });
    registry.add("import-csv", [] { return std::make_unique<ImportCsvAction>(); });
    registry.add("quit", [] { return std::make_unique<QuitAction>(); });
    registry.add("force-quit", [] { return std::make_unique<ForceQuitAction>(); });

    // короткие имена для командной строки, как в vim
    registry.add_alias("w", "save");
    registry.add_alias("e", "open");
    registry.add_alias("q", "quit");
    registry.add_alias("q!", "force-quit");
    registry.add_alias("import", "import-csv");
    registry.add_alias("help", "show-help");
    registry.add_alias("u", "undo");
    registry.add_alias("ir", "insert-row");
    registry.add_alias("dr", "delete-row");
    registry.add_alias("ic", "insert-col");
    registry.add_alias("dc", "delete-col");
}

}  // namespace ss::app
