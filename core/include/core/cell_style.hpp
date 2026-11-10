#pragma once

#include <optional>
#include <string>

namespace ss::core {

enum class Align { Left, Right, Center };

// Как показать ячейку. На значение и на формулы не влияет.
struct CellStyle {
    // формат чисел, например "fixed:2"; пустая строка значит обычный
    std::string format;
    // пусто значит само: числа вправо, остальное влево
    std::optional<Align> align;

    bool operator==(const CellStyle&) const = default;
};

}  // namespace ss::core
