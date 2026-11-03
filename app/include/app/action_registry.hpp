#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "app/action.hpp"

namespace ss::app {

// Действия по именам (паттерн Factory). Новое действие это новый класс
// и строка в register_actions, сам реестр не меняется.
class ActionRegistry final {
public:
    using Factory = std::function<std::unique_ptr<Action>()>;

    // имя уже занято (или это алиас) -> ss::core::Error
    void add(std::string name, Factory factory);
    // короткое имя для командной строки, например "w" для "save"
    void add_alias(std::string alias, std::string_view name);

    // понимает и алиасы; нет такого -> CommandError
    std::unique_ptr<Action> create(std::string_view name) const;
    // только настоящие имена, без алиасов, по алфавиту
    std::vector<std::string> names() const;

private:
    std::map<std::string, Factory, std::less<>> factories_;
    // алиас -> настоящее имя
    std::map<std::string, std::string, std::less<>> aliases_;
};

}  // namespace ss::app
