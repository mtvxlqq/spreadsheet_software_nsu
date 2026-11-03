#pragma once

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace ss::core {

class CellSource;

// Формат, который умеет записать таблицу в файл (паттерн Strategy).
class FileWriter {
public:
    virtual ~FileWriter() = default;
    FileWriter(const FileWriter&) = delete;
    FileWriter& operator=(const FileWriter&) = delete;
    FileWriter(FileWriter&&) = delete;
    FileWriter& operator=(FileWriter&&) = delete;

    virtual std::string_view name() const = 0;
    // в нижнем регистре и с точкой, например ".sst"
    virtual std::vector<std::string> extensions() const = 0;
    // если поток не дал записать, бросает FileFormatError
    virtual void save(std::ostream& out, const CellSource& source) const = 0;

protected:
    FileWriter() = default;
};

}  // namespace ss::core
