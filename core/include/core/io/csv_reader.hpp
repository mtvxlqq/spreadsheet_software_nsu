#pragma once

#include "core/io/file_reader.hpp"

namespace ss::core {

// CSV по RFC 4180: поля через запятую, в кавычках можно писать запятые,
// переводы строк и "" вместо кавычки. Ячейки ставятся начиная с A1.
// Всё читается как число или текст, формулы не выполняются: "=1+1"
// станет текстом. Пустые поля пропускаются.
class CsvReader final : public FileReader {
public:
    CsvReader() = default;

    std::string_view name() const override;
    std::vector<std::string> extensions() const override;
    void load(std::istream& in, CellSink& sink) const override;
};

}  // namespace ss::core
