#include "app/modes/command_mode.hpp"

#include <memory>
#include <span>

#include "app/action_registry.hpp"
#include "app/modes/normal_mode.hpp"
#include "app/session.hpp"

namespace ss::app {

namespace {

void leave(Session& session) {
    session.edit_line().clear();
    session.set_mode(std::make_unique<NormalMode>());
}

void run(Session& session) {
    const std::string line = session.edit_line().text();
    const std::vector<std::string> words = CommandMode::split_words(line);
    if (words.empty()) {
        leave(session);
        return;
    }
    // неизвестная команда бросит здесь, и строка останется для исправления
    const std::unique_ptr<Action> action = session.actions().create(words.front());
    // Normal ставим до execute: действие может само включить другой режим
    // (":help") или заполнить строку правки, и мы не должны это затереть
    leave(session);
    try {
        action->execute(session, std::span(words).subspan(1));
    } catch (...) {
        // Session отменит смену режима, вернём и текст
        session.edit_line().set_text(line);
        throw;
    }
}

}  // namespace

std::string_view CommandMode::name() const {
    return "command";
}

void CommandMode::handle_key(Session& session, std::string_view key) const {
    if (key == "enter") {
        run(session);
    } else if (key == "esc" || (key == "backspace" && session.edit_line().text().empty())) {
        leave(session);
    } else {
        session.edit_line().handle_key(key);
    }
}

std::vector<std::string> CommandMode::split_words(std::string_view line) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : line) {
        if (c == ' ' || c == '\t') {
            if (!word.empty()) {
                words.push_back(std::move(word));
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(std::move(word));
    }
    return words;
}

}  // namespace ss::app
