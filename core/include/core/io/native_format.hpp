#pragma once

#include "core/io/file_reader.hpp"
#include "core/io/file_writer.hpp"

namespace ss::core {

// Наш формат .sst. Первая строка "SST 1", дальше по строке на ячейку:
// адрес, таб и то, что ввёл пользователь. В тексте \ пишется как \\,
// таб как \t, перевод строки как \n. Пустые строки пропускаются.
class NativeFormat final : public FileReader, public FileWriter {
public:
    NativeFormat() = default;

    // одна реализация закрывает name() и extensions() обеих баз
    std::string_view name() const override;
    std::vector<std::string> extensions() const override;

    void load(std::istream& in, CellSink& sink) const override;
    void save(std::ostream& out, const CellSource& source) const override;
};

}  // namespace ss::core
