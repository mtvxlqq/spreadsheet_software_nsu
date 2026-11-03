#pragma once

#include <string_view>

namespace ss::app {

class Session;

// Что делает клавиша, зависит от режима (паттерн State).
class Mode {
public:
    virtual ~Mode() = default;
    Mode(const Mode&) = delete;
    Mode& operator=(const Mode&) = delete;
    Mode(Mode&&) = delete;
    Mode& operator=(Mode&&) = delete;

    // "normal", "edit", "command", "help"
    virtual std::string_view name() const = 0;
    // Сменить режим можно через session.set_mode: новый режим включится
    // после возврата из этого метода, так что *this до конца жив.
    virtual void handle_key(Session& session, std::string_view key) const = 0;

    // true только у правки ячейки: тогда в строке формулы показывается
    // набираемый текст, а не содержимое ячейки
    virtual bool edits_cell() const {
        return false;
    }

protected:
    Mode() = default;
};

}  // namespace ss::app
