#include "app/register_actions.hpp"

#include <memory>
#include <string>

#include "actions/edit_actions.hpp"
#include "actions/mode_actions.hpp"
#include "actions/navigation_actions.hpp"
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
    registry.add("command-mode", [] { return std::make_unique<CommandModeAction>(); });
    registry.add("show-help", [] { return std::make_unique<ShowHelpAction>(); });
    // в фазе 11 тут появятся save, open, import-csv, quit и алиасы
}

}  // namespace ss::app
