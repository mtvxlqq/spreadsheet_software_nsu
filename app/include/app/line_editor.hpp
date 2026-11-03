#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace ss::app {

// Строка ввода с курсором: правка ячейки и командная строка.
// Курсор ходит по символам UTF-8, а не по байтам, иначе русская
// буква развалилась бы пополам.
class LineEditor {
public:
    const std::string& text() const {
        return text_;
    }

    // позиция курсора в байтах, всегда на границе символа
    std::size_t caret() const {
        return caret_;
    }

    // курсор встаёт в конец
    void set_text(std::string text);
    void clear();

    void insert(std::string_view text);
    void backspace();
    void erase_forward();
    void move_left();
    void move_right();
    void move_home();
    void move_end();

    // backspace, delete, стрелки, home, end и печатные символы.
    // false, если клавиша не про редактирование строки.
    bool handle_key(std::string_view key);

private:
    std::size_t previous_char() const;
    std::size_t next_char() const;

    std::string text_;
    std::size_t caret_ = 0;
};

}  // namespace ss::app
