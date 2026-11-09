#include "app/line_editor.hpp"

#include <utility>

#include "app/keymap.hpp"
#include "core/text.hpp"

namespace ss::app {

void LineEditor::set_text(std::string text) {
    text_ = std::move(text);
    caret_ = text_.size();
}

void LineEditor::clear() {
    text_.clear();
    caret_ = 0;
}

void LineEditor::insert(std::string_view text) {
    text_.insert(caret_, text);
    caret_ += text.size();
}

void LineEditor::backspace() {
    const std::size_t start = previous_char();
    text_.erase(start, caret_ - start);
    caret_ = start;
}

void LineEditor::erase_forward() {
    text_.erase(caret_, next_char() - caret_);
}

void LineEditor::move_left() {
    caret_ = previous_char();
}

void LineEditor::move_right() {
    caret_ = next_char();
}

void LineEditor::move_home() {
    caret_ = 0;
}

void LineEditor::move_end() {
    caret_ = text_.size();
}

bool LineEditor::handle_key(std::string_view key) {
    if (key == "backspace") {
        backspace();
    } else if (key == "delete") {
        erase_forward();
    } else if (key == "left") {
        move_left();
    } else if (key == "right") {
        move_right();
    } else if (key == "home") {
        move_home();
    } else if (key == "end") {
        move_end();
    } else if (is_text_key(key)) {
        insert(key);
    } else {
        return false;
    }
    return true;
}

std::size_t LineEditor::previous_char() const {
    std::size_t position = caret_;
    if (position == 0) {
        return 0;
    }
    --position;
    while (position > 0 && ss::core::is_utf8_continuation(text_[position])) {
        --position;
    }
    return position;
}

std::size_t LineEditor::next_char() const {
    std::size_t position = caret_;
    if (position == text_.size()) {
        return position;
    }
    ++position;
    while (position < text_.size() && ss::core::is_utf8_continuation(text_[position])) {
        ++position;
    }
    return position;
}

}  // namespace ss::app
