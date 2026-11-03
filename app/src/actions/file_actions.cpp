#include "actions/file_actions.hpp"

#include <filesystem>
#include <string>

#include "app/document_io.hpp"
#include "app/errors.hpp"
#include "app/session.hpp"
#include "core/io/format_registry.hpp"

namespace ss::app {

namespace {

// ровно один аргумент, путь к файлу
std::filesystem::path path_argument(std::string_view name, std::span<const std::string> args) {
    if (args.size() != 1) {
        throw CommandError("нужно указать один файл: " + std::string(name) + " путь");
    }
    return args.front();
}

}  // namespace

std::string_view SaveAction::name() const {
    return "save";
}

std::string_view SaveAction::description() const {
    return "сохранить, можно указать файл (:w путь)";
}

void SaveAction::execute(Session& session, std::span<const std::string> args) const {
    if (args.size() > 1) {
        throw CommandError("save принимает только один файл");
    }
    std::filesystem::path path;
    if (!args.empty()) {
        path = args.front();
    } else if (session.current_path()) {
        path = *session.current_path();
    } else {
        throw CommandError("у документа ещё нет файла, нужно :w путь");
    }
    // если такого формата нет, бросит до того, как мы что-то запишем
    const ss::core::FileWriter& writer = session.formats().writer_for(path.string());
    save_document(path, writer, session.document());
    session.mark_saved(path);
    session.set_status("сохранено в " + path.string());
}

std::string_view OpenAction::name() const {
    return "open";
}

std::string_view OpenAction::description() const {
    return "открыть файл (:e путь)";
}

void OpenAction::execute(Session& session, std::span<const std::string> args) const {
    const std::filesystem::path path = path_argument(name(), args);
    const ss::core::FileReader& reader = session.formats().reader_for(path.string());
    // грузим в новую таблицу, старая заменится только если всё прочиталось
    session.replace_document(load_document(path, reader, session.functions()), path);
    session.set_status("открыт " + path.string());
}

std::string_view ImportCsvAction::name() const {
    return "import-csv";
}

std::string_view ImportCsvAction::description() const {
    return "загрузить CSV, формулы из него не выполняются (:import путь)";
}

void ImportCsvAction::execute(Session& session, std::span<const std::string> args) const {
    const std::filesystem::path path = path_argument(name(), args);
    const ss::core::FileReader& reader = session.formats().reader_for(path.string());
    session.import_document(load_document(path, reader, session.functions()));
    session.set_status("загружен " + path.string() + ", сохраните через :w путь");
}

}  // namespace ss::app
