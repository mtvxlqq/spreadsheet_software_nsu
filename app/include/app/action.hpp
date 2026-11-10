#pragma once

#include <span>
#include <string>
#include <string_view>

namespace ss::app {

class Session;

// То, что можно запустить клавишей или командой ":имя арг" (паттерн Command).
// Действия ничего не хранят, всё состояние лежит в Session.
class Action {
public:
    virtual ~Action() = default;
    Action(const Action&) = delete;
    Action& operator=(const Action&) = delete;
    Action(Action&&) = delete;
    Action& operator=(Action&&) = delete;

    virtual std::string_view name() const = 0;
    // одна строка для справки
    virtual std::string_view description() const = 0;
    // args это слова после имени в командной строке, от клавиши их нет.
    // Ошибки бросает как ss::core::Error, Session покажет их в статусе.
    virtual void execute(Session& session, std::span<const std::string> args) const = 0;

    // Остаётся ли выделение после действия. Так у перемещений: ими выделение
    // и растягивают. Обычное действие работает с выделенным диапазоном
    // и снимает его, поэтому новому действию тут ничего менять не нужно.
    virtual bool keeps_selection() const {
        return false;
    }

protected:
    Action() = default;
};

}  // namespace ss::app
