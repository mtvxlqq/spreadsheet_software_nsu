#include "app/action_registry.hpp"

#include <utility>

#include "app/errors.hpp"

namespace ss::app {

void ActionRegistry::add(std::string name, Factory factory) {
    if (factories_.contains(name) || aliases_.contains(name)) {
        throw ss::core::Error("действие " + name + " уже зарегистрировано");
    }
    factories_.emplace(std::move(name), std::move(factory));
}

void ActionRegistry::add_alias(std::string alias, std::string_view name) {
    if (!factories_.contains(name)) {
        throw ss::core::Error("алиас " + alias + " для неизвестного действия " +
                              std::string(name));
    }
    if (factories_.contains(alias) || aliases_.contains(alias)) {
        throw ss::core::Error("имя " + alias + " уже занято");
    }
    aliases_.emplace(std::move(alias), std::string(name));
}

std::unique_ptr<Action> ActionRegistry::create(std::string_view name) const {
    const auto alias = aliases_.find(name);
    const std::string_view real_name = alias == aliases_.end() ? name : alias->second;
    const auto it = factories_.find(real_name);
    if (it == factories_.end()) {
        throw CommandError("нет такой команды: " + std::string(name));
    }
    return it->second();
}

std::vector<std::string> ActionRegistry::names() const {
    std::vector<std::string> result;
    result.reserve(factories_.size());
    for (const auto& [name, factory] : factories_) {
        result.push_back(name);
    }
    return result;
}

}  // namespace ss::app
