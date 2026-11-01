#include "core/dependency_graph.hpp"

#include <utility>

namespace ss::core {

namespace {

std::set<CellAddress> find_or_empty(const std::map<CellAddress, std::set<CellAddress>>& edges,
                                    CellAddress cell) {
    const auto it = edges.find(cell);
    return it == edges.end() ? std::set<CellAddress>{} : it->second;
}

// убирает ребро from -> to, а пустое множество удаляет целиком
void erase_edge(std::map<CellAddress, std::set<CellAddress>>& edges, CellAddress from,
                CellAddress to) noexcept {
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

// Сначала делаем всё, что может бросить (вставки в map и set), и при ошибке
// откатываем то, что успели вставить. Удаления старых рёбер и swap не бросают,
// их делаем в самом конце.
void DependencyGraph::set_dependencies(CellAddress cell, std::set<CellAddress> dependencies) {
    if (dependencies.empty()) {
        remove_cell(cell);
        return;
    }
    const std::set<CellAddress> old_dependencies = find_or_empty(dependencies_, cell);

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

    // дальше ничего не бросает
    current->swap(dependencies);
    // теперь в dependencies лежат старые зависимости ячейки
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

// обход в ширину по обратным рёбрам
std::set<CellAddress> DependencyGraph::collect_affected(
    const std::vector<CellAddress>& changed) const {
    std::set<CellAddress> affected(changed.begin(), changed.end());
    std::vector<CellAddress> queue(changed.begin(), changed.end());
    while (!queue.empty()) {
        const CellAddress cell = queue.back();
        queue.pop_back();
        const auto it = dependents_.find(cell);
        if (it == dependents_.end()) {
            continue;
        }
        for (const CellAddress dependent : it->second) {
            if (affected.insert(dependent).second) {
                queue.push_back(dependent);
            }
        }
    }
    return affected;
}

// Алгоритм Кана на затронутых ячейках: берём ячейку, у которой все
// зависимости уже посчитаны, и так далее. Что осталось, лежит в цикле
// или зависит от цикла.
RecalcPlan DependencyGraph::plan(const std::vector<CellAddress>& changed) const {
    const std::set<CellAddress> affected = collect_affected(changed);

    // сколько ещё не посчитанных зависимостей у каждой затронутой ячейки
    std::map<CellAddress, int> waiting;
    for (const CellAddress cell : affected) {
        int count = 0;
        for (const CellAddress dependency : find_or_empty(dependencies_, cell)) {
            if (affected.contains(dependency)) {
                ++count;
            }
        }
        waiting[cell] = count;
    }

    // set вместо очереди, чтобы порядок не зависел от случайностей
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
        const auto it = dependents_.find(cell);
        if (it == dependents_.end()) {
            continue;
        }
        for (const CellAddress dependent : it->second) {
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
    return find_or_empty(dependencies_, cell);
}

std::set<CellAddress> DependencyGraph::dependents_of(CellAddress cell) const {
    return find_or_empty(dependents_, cell);
}

}  // namespace ss::core
