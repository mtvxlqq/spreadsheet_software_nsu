#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

namespace ss::core {

// Корень всех ошибок предметной области. Слой приложения ловит именно его:
// сообщение попадает в строку состояния, а программа продолжает работу.
class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Неверный текст адреса: "A0", "1A", "ZZZ99999999".
class AddressError : public Error {
public:
    using Error::Error;
};

// Синтаксическая ошибка в формуле.
class ParseError : public Error {
public:
    // position — смещение в байтах от начала формулы, с нуля.
    // В тексте сообщения позиция печатается с единицы, как привычно человеку.
    ParseError(const std::string& message, std::size_t position)
        : Error("ошибка в формуле, позиция " + std::to_string(position + 1) + ": " + message),
          position_(position) {}

    std::size_t position() const noexcept {
        return position_;
    }

private:
    std::size_t position_;
};

// Повреждённый файл таблицы или CSV, неизвестный формат файла.
class FileFormatError : public Error {
public:
    // Ошибка, не привязанная к строке файла: line() == 0.
    explicit FileFormatError(const std::string& message) : Error(message), line_(0) {}

    // line — номер строки файла, с единицы.
    FileFormatError(const std::string& message, std::size_t line)
        : Error("строка " + std::to_string(line) + ": " + message), line_(line) {}

    std::size_t line() const noexcept {
        return line_;
    }

private:
    std::size_t line_;
};

// Операция выходит за размеры листа.
class LimitError : public Error {
public:
    using Error::Error;
};

}  // namespace ss::core
