#include "actions/file_actions.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>

#include "app/document_io.hpp"
#include "app/errors.hpp"
#include "app/session.hpp"
#include "core/io/format_registry.hpp"
#include "core/spreadsheet.hpp"

namespace ss::app {

namespace {

// Загружает файл в фоне. Формат проверяем сразу: если его нет,
// ошибка будет видна без всякого потока.
void load_in_background(Session& session, const std::filesystem::path& path,
                        std::function<void(Session&, std::unique_ptr<ss::core::Spreadsheet>)> done) {
    const ss::core::FileReader& reader = session.formats().reader_for(path.string());
    const ss::core::FunctionProvider& functions = session.functions();
    const ss::core::ValueParserRegistry& value_parsers = session.value_parsers();
    session.set_status("загрузка " + path.string() + "...");
    // В фоне трогаем только reader и реестры, их никто не меняет,
    // так что читать из двух потоков можно. Сеанс тут не используется.
    session.run_in_background([path, &reader, &functions, &value_parsers, done = std::move(done)](
                                  std::stop_token) -> std::function<void(Session&)> {
        // std::function нужна копируемая лямбда, а unique_ptr не копируется,
        // поэтому таблицу кладём в shared_ptr и забираем из него в done
        auto document = std::make_shared<std::unique_ptr<ss::core::Spreadsheet>>(
            load_document(path, reader, functions, value_parsers));
        return [document, done](Session& target) { done(target, std::move(*document)); };
    });
}

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
    if (!writer.keeps_formulas()) {
        // иначе формулы пропали бы, а файл без них стал бы текущим документом
        throw CommandError("формат " + std::string(writer.name()) +
                           " не хранит формулы, для него есть :export путь");
    }
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
    // грузим в новую таблицу, старая заменится только если всё прочиталось
    load_in_background(session, path, [path](Session& target, std::unique_ptr<ss::core::Spreadsheet> document) {
        target.replace_document(std::move(document), path);
        target.set_status("открыт " + path.string());
    });
}

std::string_view ExportAction::name() const {
    return "export";
}

std::string_view ExportAction::description() const {
    return "записать значения без формул, например в CSV (:export путь)";
}

void ExportAction::execute(Session& session, std::span<const std::string> args) const {
    const std::filesystem::path path = path_argument(name(), args);
    const ss::core::FileWriter& writer = session.formats().writer_for(path.string());
    // та же запись через временный файл, что и при сохранении
    save_document(path, writer, session.document());
    session.set_status("экспортировано в " + path.string());
}

std::string_view ImportCsvAction::name() const {
    return "import-csv";
}

std::string_view ImportCsvAction::description() const {
    return "загрузить CSV, формулы из него не выполняются (:import путь)";
}

void ImportCsvAction::execute(Session& session, std::span<const std::string> args) const {
    const std::filesystem::path path = path_argument(name(), args);
    load_in_background(session, path, [path](Session& target, std::unique_ptr<ss::core::Spreadsheet> document) {
        target.import_document(std::move(document));
        target.set_status("загружен " + path.string() + ", сохраните через :w путь");
    });
}

}  // namespace ss::app
