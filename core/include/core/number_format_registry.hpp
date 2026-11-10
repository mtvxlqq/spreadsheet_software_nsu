#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "core/number_format.hpp"

namespace ss::core {

// Форматы чисел по имени (паттерн Factory). В стиле ячейки формат записан
// строкой: "fixed:2" это имя и число знаков, пустая строка это обычный
// формат. Новый формат это новый класс и строка в register_number_formats.
class NumberFormatRegistry final {
public:
    // digits это число после двоеточия
    using Factory = std::function<std::shared_ptr<const NumberFormat>(int digits)>;

    // больше знаков у double всё равно нет
    static constexpr int max_digits = 15;

    // если имя уже занято, бросает Error
    void add(std::string name, Factory factory);

    // Формат по строке из стиля. Пустая строка, неизвестное имя и кривое число
    // знаков дают обычный формат: файл мог сохранить кто-то с другим набором
    // форматов, и это не повод его не открывать.
    std::shared_ptr<const NumberFormat> create(std::string_view spec) const;

    // Строка для стиля: ("fixed", 2) даёт "fixed:2". Если такого формата нет
    // или знаков не от 0 до max_digits, бросает Error.
    std::string make_spec(std::string_view name, int digits) const;

    // по алфавиту
    std::vector<std::string> names() const;

private:
    std::map<std::string, Factory, std::less<>> factories_;
    std::shared_ptr<const NumberFormat> general_ = std::make_shared<const GeneralFormat>();
};

// Регистрирует fixed и percent. Новый встроенный формат: свой класс
// в number_format.hpp и одна строка здесь.
void register_number_formats(NumberFormatRegistry& registry);

}  // namespace ss::core
