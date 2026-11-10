#pragma once

#include "core/io/file_writer.hpp"

namespace ss::core {

// Экспорт в CSV по RFC 4180. Пишутся значения, а не формулы: "=A1*2"
// попадёт в файл своим результатом, ошибка текстом вроде #DIV/0!.
// В файле прямоугольник от A1 до правого нижнего угла заполненной части,
// пустые ячейки это пустые поля. Оформление в CSV не попадает.
class CsvWriter final : public FileWriter {
public:
    CsvWriter() = default;

    std::string_view name() const override;
    std::vector<std::string> extensions() const override;
    void save(std::ostream& out, const CellSource& source) const override;

    // формулы и оформление теряются, поэтому в CSV можно только экспортировать
    bool keeps_formulas() const override {
        return false;
    }
};

}  // namespace ss::core
