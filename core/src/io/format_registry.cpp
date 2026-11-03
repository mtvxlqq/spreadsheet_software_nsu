#include "core/io/format_registry.hpp"

#include <filesystem>
#include <utility>

#include "core/errors.hpp"
#include "core/text.hpp"

namespace ss::core {

namespace {

// Сначала добавляем в копию и только потом подменяем, чтобы при ошибке
// не остался формат, зарегистрированный наполовину.
template <typename Format>
void add_format(std::map<std::string, std::shared_ptr<const Format>>& formats,
                std::shared_ptr<const Format> format) {
    auto updated = formats;
    for (const std::string& extension : format->extensions()) {
        std::string key = to_lower_ascii(extension);
        if (updated.contains(key)) {
            throw Error("формат для " + key + " уже зарегистрирован");
        }
        updated.emplace(std::move(key), format);
    }
    formats.swap(updated);
}

template <typename Format>
const Format& find_format(const std::map<std::string, std::shared_ptr<const Format>>& formats,
                          std::string_view path) {
    const std::string extension =
        to_lower_ascii(std::filesystem::path(path).extension().string());
    if (extension.empty()) {
        throw FileFormatError("у файла " + std::string(path) + " нет расширения");
    }
    const auto it = formats.find(extension);
    if (it == formats.end()) {
        throw FileFormatError("не знаю формат " + extension);
    }
    return *it->second;
}

}  // namespace

void FormatRegistry::add_reader(std::shared_ptr<const FileReader> reader) {
    add_format(readers_, std::move(reader));
}

void FormatRegistry::add_writer(std::shared_ptr<const FileWriter> writer) {
    add_format(writers_, std::move(writer));
}

const FileReader& FormatRegistry::reader_for(std::string_view path) const {
    return find_format(readers_, path);
}

const FileWriter& FormatRegistry::writer_for(std::string_view path) const {
    return find_format(writers_, path);
}

}  // namespace ss::core
