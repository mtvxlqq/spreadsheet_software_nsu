#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "core/function.hpp"

namespace ss::core {

// Функции по именам (паттерн Factory). Новая функция добавляется новым
// классом и одной строкой в register_builtins, этот класс не меняется.
class FunctionRegistry final : public FunctionProvider {
public:
    using Factory = std::function<std::shared_ptr<const Function>()>;

    // имя без учёта регистра; если такое уже есть, бросает Error
    void add(std::string name, Factory factory);

    std::shared_ptr<const Function> create(std::string_view name) const override;

private:
    // ключ в верхнем регистре
    std::map<std::string, Factory> factories_;
};

}  // namespace ss::core
