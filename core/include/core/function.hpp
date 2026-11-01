#pragma once

#include <memory>
#include <span>
#include <string_view>

#include "core/value.hpp"

namespace ss::core {

class EvalContext;
class Expr;

// Функция формулы, например SUM или IF. Состояния у функций нет, поэтому один
// объект спокойно делят все формулы, где она встречается.
class Function {
public:
    virtual ~Function() = default;
    Function(const Function&) = delete;
    Function& operator=(const Function&) = delete;
    Function(Function&&) = delete;
    Function& operator=(Function&&) = delete;

    virtual std::string_view name() const = 0;

    // Аргументы приходят невычисленными: IF должен посчитать только одну ветку.
    // Ошибки возвращаются значением, исключения тут не бросаются.
    virtual ValuePtr call(std::span<const Expr* const> args, EvalContext& context) const = 0;

protected:
    Function() = default;
};

// Всё, что парсеру нужно знать о функциях: найти функцию по имени.
// Добавлять функции он не умеет, это делает реестр.
class FunctionProvider {
public:
    virtual ~FunctionProvider() = default;
    FunctionProvider(const FunctionProvider&) = delete;
    FunctionProvider& operator=(const FunctionProvider&) = delete;
    FunctionProvider(FunctionProvider&&) = delete;
    FunctionProvider& operator=(FunctionProvider&&) = delete;

    // имя без учёта регистра; если такой функции нет, вернёт nullptr
    virtual std::shared_ptr<const Function> create(std::string_view name) const = 0;

protected:
    FunctionProvider() = default;
};

}  // namespace ss::core
