#include "app/register_actions.hpp"

#include <memory>
#include <string>

#include "actions/edit_actions.hpp"
#include "actions/file_actions.hpp"
#include "actions/mode_actions.hpp"
#include "actions/navigation_actions.hpp"
#include "actions/quit_actions.hpp"
#include "app/action_registry.hpp"

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
}

}  // namespace ss::app
