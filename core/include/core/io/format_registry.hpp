#pragma once

#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "core/io/file_reader.hpp"
#include "core/io/file_writer.hpp"

namespace ss::core {

// Форматы по расширению файла. Новый формат это новый класс и строка
// в register_formats, сам реестр не меняется.
class FormatRegistry final {
public:
    // shared_ptr, потому что NativeFormat регистрируется и как читатель,
    // и как писатель. Если расширение уже занято, бросает Error.
    void add_reader(std::shared_ptr<const FileReader> reader);
    void add_writer(std::shared_ptr<const FileWriter> writer);

    // расширение без учёта регистра; нет подходящего формата -> FileFormatError
    const FileReader& reader_for(std::string_view path) const;
    const FileWriter& writer_for(std::string_view path) const;

private:
    // ключ это расширение в нижнем регистре
    std::map<std::string, std::shared_ptr<const FileReader>> readers_;
    std::map<std::string, std::shared_ptr<const FileWriter>> writers_;
};

}  // namespace ss::core
