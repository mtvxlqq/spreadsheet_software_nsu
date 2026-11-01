#pragma once

#include <vector>

#include "core/address.hpp"

namespace ss::core {

class EvalContext;
class Sheet;
struct RecalcPlan;

// Пересчитывает формулы строго по плану из DependencyGraph. Когда очередь
// доходит до ячейки, её зависимости уже свежие, поэтому каждая формула
// считается один раз и без рекурсии.
class Recalculator {
public:
    // Возвращает все адреса из плана, сначала order, потом cyclic: это то,
    // о чём надо сообщить наблюдателям. Исключений не бросает, кроме bad_alloc.
    static std::vector<CellAddress> run(const RecalcPlan& plan, Sheet& sheet,
                                        EvalContext& context);
};

}  // namespace ss::core
