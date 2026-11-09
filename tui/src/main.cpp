#include <array>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/app.hpp>

#include "app/action.hpp"
#include "app/action_registry.hpp"
#include "app/keymap.hpp"
#include "app/register_actions.hpp"
#include "app/session.hpp"
#include "app/status.hpp"
#include "core/builtins.hpp"
#include "core/errors.hpp"
#include "core/function_registry.hpp"
#include "core/io/format_registry.hpp"
#include "core/io/register_formats.hpp"
#include "core/number_format.hpp"
#include "tui/ftxui_dispatcher.hpp"
#include "tui/tui_app.hpp"

namespace {

using ss::app::Session;
using ss::app::Status;

// пустой путь значит, что конфига нет, тогда клавиши будут по умолчанию
std::filesystem::path keys_config_path() {
    const std::filesystem::path relative = std::filesystem::path("spreadsheet") / "keys.conf";
    const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
    if (xdg_config != nullptr && *xdg_config != '\0') {
        return xdg_config / relative;
    }
    const char* home = std::getenv("HOME");
    if (home != nullptr && *home != '\0') {
        return std::filesystem::path(home) / ".config" / relative;
    }
    return {};
}

// в строке состояния место одно, так что первое предупреждение и сколько ещё
std::string warnings_text(const std::vector<std::string>& warnings) {
    std::string text = "конфиг клавиш, " + warnings.front();
    if (warnings.size() > 1) {
        text += " (и ещё " + std::to_string(warnings.size() - 1) + ")";
    }
    return text;
}

// Файл открывает то же действие, что и команда :open. Если не получилось,
// ошибка видна в статусе, а программа работает с пустой таблицей.
void open_file(Session& session, const std::string& path) {
    const std::array<std::string, 1> args{path};
    try {
        session.actions().create("open")->execute(session, args);
    } catch (const ss::core::Error& error) {
        session.set_status(error.what(), Status::Kind::Error);
    }
}

int run_program(int argc, char** argv) {
    if (argc > 2) {
        std::cerr << "использование: spreadsheet [файл]\n";
        return 2;
    }

    ss::core::FunctionRegistry functions;
    ss::core::register_builtins(functions);
    ss::core::FormatRegistry formats;
    ss::core::register_formats(formats);
    ss::app::ActionRegistry actions;
    ss::app::register_actions(actions);
    ss::core::GeneralFormat format;

    ss::app::KeymapLoadResult keymap = ss::app::Keymap::load(keys_config_path(), actions.names());

    // Порядок важен. Сеанс удалится раньше App и в деструкторе дождётся
    // фоновых потоков, так что они не обратятся к уже удалённому App.
    ftxui::App app = ftxui::App::Fullscreen();
    ss::tui::FtxuiDispatcher dispatcher(app);
    Session session(functions, formats, actions, std::move(keymap.keymap), dispatcher);
    ss::tui::TuiApp tui(app, session, format);

    if (!keymap.warnings.empty()) {
        session.set_status(warnings_text(keymap.warnings), Status::Kind::Error);
    }
    if (argc == 2) {
        open_file(session, argv[1]);
    }
    tui.run();
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run_program(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "ошибка: " << error.what() << '\n';
        return 1;
    }
}
