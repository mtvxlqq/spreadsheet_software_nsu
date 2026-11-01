#pragma once

#include <map>
#include <set>
#include <vector>

#include "core/address.hpp"

namespace ss::core {

// Что пересчитать и в каком порядке.
struct RecalcPlan {
    // к моменту вычисления ячейки все её зависимости уже стоят раньше
    std::vector<CellAddress> order;
    // ячейки в цикле и всё, что от цикла зависит, им ставится #CYCLE!
    std::set<CellAddress> cyclic;
};

// Кто от кого зависит. Значения не вычисляет.
class DependencyGraph {
public:
    // Заменяет зависимости ячейки на новые. Строгая гарантия: если бросит
    // bad_alloc, граф останется как был.
    void set_dependencies(CellAddress cell, std::set<CellAddress> dependencies);

    // ячейка больше ни от чего не зависит (её очистили или там уже не формула)
    void remove_cell(CellAddress cell) noexcept;

    // Порядок пересчёта после изменения changed. В план попадают сами changed
    // и всё, что от них зависит, прямо или через другие ячейки.
    RecalcPlan plan(const std::vector<CellAddress>& changed) const;

    // от чего зависит ячейка и кто зависит от неё (для тестов и отладки)
    std::set<CellAddress> dependencies_of(CellAddress cell) const;
    std::set<CellAddress> dependents_of(CellAddress cell) const;

private:
    std::set<CellAddress> collect_affected(const std::vector<CellAddress>& changed) const;

    // ячейка -> на кого ссылается её формула
    std::map<CellAddress, std::set<CellAddress>> dependencies_;
    // ячейка -> чьи формулы на неё ссылаются (обратные рёбра)
    std::map<CellAddress, std::set<CellAddress>> dependents_;
};

}  // namespace ss::core
