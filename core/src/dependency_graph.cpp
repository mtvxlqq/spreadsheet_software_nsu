#include "core/dependency_graph.hpp"

#include <utility>

namespace ss::core {

namespace {

using Edges = std::map<CellAddress, std::set<CellAddress>>;

// соседи ячейки без копирования; у кого рёбер нет, тем общий пустой set
const std::set<CellAddress>& edges_of(const Edges& edges, CellAddress cell) {
    static const std::set<CellAddress> none;
    const auto it = edges.find(cell);
    return it == edges.end() ? none : it->second;
}

// Убирает одно ребро. Если у from рёбер не осталось, убираем и его,
// чтобы в map не копились пустые записи.
void erase_edge(Edges& edges, CellAddress from, CellAddress to) noexcept {
    const auto it = edges.find(from);
    if (it == edges.end()) {
        return;
    }
    it->second.erase(to);
    if (it->second.empty()) {
        edges.erase(it);
    }
}

}  // namespace

// Вставки в map и set могут бросить bad_alloc, поэтому делаем их первыми
// и при ошибке убираем то, что успели вставить. Всё, что после них,
// бросить уже не может, так что граф не останется сломанным наполовину.
void DependencyGraph::set_dependencies(CellAddress cell, std::set<CellAddress> dependencies) {
    if (dependencies.empty()) {
        remove_cell(cell);
        return;
    }
    const std::set<CellAddress>& old_dependencies = edges_of(dependencies_, cell);

    std::vector<CellAddress> added;
    std::set<CellAddress>* current = nullptr;
    try {
        added.reserve(dependencies.size());
        for (const CellAddress dependency : dependencies) {
            if (!old_dependencies.contains(dependency)) {
                dependents_[dependency].insert(cell);
                added.push_back(dependency);
            }
        }
        current = &dependencies_[cell];
    } catch (...) {
        for (const CellAddress dependency : added) {
            erase_edge(dependents_, dependency, cell);
        }
        throw;
    }

    current->swap(dependencies);
    // после swap в dependencies лежат старые зависимости
    for (const CellAddress old_dependency : dependencies) {
        if (!current->contains(old_dependency)) {
            erase_edge(dependents_, old_dependency, cell);
        }
    }
}

void DependencyGraph::remove_cell(CellAddress cell) noexcept {
    const auto it = dependencies_.find(cell);
    if (it == dependencies_.end()) {
        return;
    }
    for (const CellAddress dependency : it->second) {
        erase_edge(dependents_, dependency, cell);
    }
    dependencies_.erase(it);
}

// Идём по обратным рёбрам со своим стеком, а не рекурсией: цепочка
// A2=A1+1, A3=A2+1, ... на весь столбец переполнила бы стек вызовов.
std::set<CellAddress> DependencyGraph::collect_affected(
    const std::vector<CellAddress>& changed) const {
    std::set<CellAddress> affected(changed.begin(), changed.end());
    std::vector<CellAddress> to_visit(changed.begin(), changed.end());
    while (!to_visit.empty()) {
        const CellAddress cell = to_visit.back();
        to_visit.pop_back();
        for (const CellAddress dependent : edges_of(dependents_, cell)) {
            if (affected.insert(dependent).second) {
                to_visit.push_back(dependent);
            }
        }
    }
    return affected;
}

// Алгоритм Кана: каждый раз берём ячейку, у которой все зависимости уже
// посчитаны. Кто так и не дождался своей очереди, сидит в цикле или ниже него.
RecalcPlan DependencyGraph::plan(const std::vector<CellAddress>& changed) const {
    const std::set<CellAddress> affected = collect_affected(changed);

    // сколько ещё не посчитанных зависимостей у каждой затронутой ячейки
    std::map<CellAddress, int> waiting;
    for (const CellAddress cell : affected) {
        int count = 0;
        for (const CellAddress dependency : edges_of(dependencies_, cell)) {
            if (affected.contains(dependency)) {
                ++count;
            }
        }
        waiting[cell] = count;
    }

    // set, а не очередь: так порядок всегда один и тот же, и его можно проверить в тестах
    std::set<CellAddress> ready;
    for (const auto& [cell, count] : waiting) {
        if (count == 0) {
            ready.insert(cell);
        }
    }

    RecalcPlan result;
    while (!ready.empty()) {
        const CellAddress cell = *ready.begin();
        ready.erase(ready.begin());
        result.order.push_back(cell);
        for (const CellAddress dependent : edges_of(dependents_, cell)) {
            if (--waiting[dependent] == 0) {
                ready.insert(dependent);
            }
        }
    }

    for (const auto& [cell, count] : waiting) {
        if (count > 0) {
            result.cyclic.insert(cell);
        }
    }
    return result;
}

std::set<CellAddress> DependencyGraph::dependencies_of(CellAddress cell) const {
    return edges_of(dependencies_, cell);
}

std::set<CellAddress> DependencyGraph::dependents_of(CellAddress cell) const {
    return edges_of(dependents_, cell);
}

}  // namespace ss::core
