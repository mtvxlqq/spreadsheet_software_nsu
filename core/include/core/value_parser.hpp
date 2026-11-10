#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "core/value.hpp"

namespace ss::core {

// Узнаёт в том, что ввели в ячейку, значение своего типа (паттерн Strategy).
// Новый тип значения, который можно ввести прямо в ячейку, это наследник
// Value, такой разборщик для него и строка в register_value_parsers.
class ValueParser {
public:
    virtual ~ValueParser() = default;
    ValueParser(const ValueParser&) = delete;
    ValueParser& operator=(const ValueParser&) = delete;
    ValueParser(ValueParser&&) = delete;
    ValueParser& operator=(ValueParser&&) = delete;

    // text уже без пробелов по краям; nullptr, если это не его тип
    virtual ValuePtr parse(std::string_view text) const = 0;

protected:
    ValueParser() = default;
};

// Разборщики по очереди пробуют ввод, первый узнавший побеждает.
// Никто не узнал: значит, в ячейке просто текст.
class ValueParserRegistry final {
public:
    // порядок важен: кто добавлен раньше, тот и пробует раньше
    void add(std::shared_ptr<const ValueParser> parser);

    // nullptr, если ввод не узнал никто
    ValuePtr parse(std::string_view text) const;

private:
    std::vector<std::shared_ptr<const ValueParser>> parsers_;
};

// Регистрирует числа и даты. Новый встроенный тип: свой разборщик
// в value_parsers.cpp и одна строка здесь.
void register_value_parsers(ValueParserRegistry& registry);

}  // namespace ss::core
