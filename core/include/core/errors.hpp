#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

namespace ss::core {

// Базовый класс для всех наших ошибок. app ловит его и показывает текст
// в строке состояния, программа при этом работает дальше.
class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// например "A0", "1A" или "ZZZ99999999"
class AddressError : public Error {
public:
    using Error::Error;
};

class ParseError : public Error {
public:
    // position считаем с нуля (в байтах), а в сообщении пишем с единицы
    ParseError(const std::string& message, std::size_t position)
        : Error("ошибка в формуле, позиция " + std::to_string(position + 1) + ": " + message),
          position_(position) {}

    std::size_t position() const noexcept {
        return position_;
    }

private:
    std::size_t position_;
};

// битый файл или неизвестное расширение
class FileFormatError : public Error {
public:
    // когда номер строки не важен, line() будет 0
    explicit FileFormatError(const std::string& message) : Error(message), line_(0) {}

    FileFormatError(const std::string& message, std::size_t line)
        : Error("строка " + std::to_string(line) + ": " + message), line_(line) {}

    std::size_t line() const noexcept {
        return line_;
    }

private:
    std::size_t line_;
};

// выход за пределы листа
class LimitError : public Error {
public:
    using Error::Error;
};

}  // namespace ss::core
