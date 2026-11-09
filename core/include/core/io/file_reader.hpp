#pragma once

#include <istream>
#include <string>
#include <string_view>
#include <vector>

namespace ss::core {

class CellSink;

// Формат, который умеет читать файл (паттерн Strategy). Читать и писать
// разделено, потому что CSV на первом этапе только читается.
class FileReader {
public:
    virtual ~FileReader() = default;
    FileReader(const FileReader&) = delete;
    FileReader& operator=(const FileReader&) = delete;
    FileReader(FileReader&&) = delete;
    FileReader& operator=(FileReader&&) = delete;

    virtual std::string_view name() const = 0;
    // в нижнем регистре и с точкой, например ".sst"
    virtual std::vector<std::string> extensions() const = 0;
    // На битом файле бросает FileFormatError с номером строки. Что успело попасть
    // в sink до ошибки, там и останется, поэтому грузить надо в новую таблицу.
    virtual void load(std::istream& in, CellSink& sink) const = 0;

protected:
    FileReader() = default;
};

}  // namespace ss::core
