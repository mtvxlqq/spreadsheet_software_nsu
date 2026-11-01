#include "core/function_registry.hpp"

#include <utility>

#include "core/errors.hpp"
#include "core/text.hpp"

namespace ss::core {

void FunctionRegistry::add(std::string name, Factory factory) {
    std::string key = to_upper_ascii(name);
    if (factories_.contains(key)) {
        throw Error("функция " + key + " уже зарегистрирована");
    }
    factories_.emplace(std::move(key), std::move(factory));
}

std::shared_ptr<const Function> FunctionRegistry::create(std::string_view name) const {
    const auto it = factories_.find(to_upper_ascii(name));
    if (it == factories_.end()) {
        return nullptr;
    }
    return it->second();
}

}  // namespace ss::core
