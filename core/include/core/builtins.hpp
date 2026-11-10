#pragma once

namespace ss::core {

class FunctionRegistry;

// Регистрирует SUM, MIN, MAX, AVERAGE, IF и DATE. Новая встроенная функция:
// свой класс в builtins/ и одна строка здесь.
void register_builtins(FunctionRegistry& registry);

}  // namespace ss::core
