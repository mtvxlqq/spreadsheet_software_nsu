#pragma once

#include <filesystem>
#include <istream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ss::app {

struct KeymapLoadResult;

// Клавиша, которая вводит текст: ровно один печатный символ UTF-8
// ("h", "?", "ж", " "). Режимы правки вставляют такие клавиши в строку.
bool is_text_key(std::string_view key);

// Какая клавиша какое действие запускает. Клавиши это строки, которые
// присылает интерфейс: "up", "enter", "ctrl+s", "h", "?". Действие
// хранится по имени, объект Action по нему потом создаёт ActionRegistry.
class Keymap final {
public:
    // пустая раскладка, обычно нужна defaults()
    Keymap() = default;

    static Keymap defaults();

    // Начинает с defaults() и накладывает сверху строки из файла. Плохие
    // строки пропускаются с предупреждением. Если файла нет, это не
    // ошибка: просто defaults() без предупреждений.
    static KeymapLoadResult load(const std::filesystem::path& path,
                                 const std::vector<std::string>& known_actions);
    // то же самое, но из потока, так проще тестировать
    static KeymapLoadResult load(std::istream& in, const std::vector<std::string>& known_actions);

    // Приводит имя клавиши к одному виду: "Ctrl+S" станет "ctrl+s".
    // nullopt, если такой клавиши нет.
    static std::optional<std::string> normalize_key(std::string_view key);

    // старая привязка этой клавиши заменяется; на непонятную клавишу бросает ConfigError
    void bind(std::string_view key, std::string action);

    std::optional<std::string> action_for(std::string_view key) const;
    // для справки: все клавиши этого действия по алфавиту
    std::vector<std::string> keys_for(std::string_view action) const;

private:
    // ключ уже нормализован
    std::map<std::string, std::string> actions_;
};

struct KeymapLoadResult {
    Keymap keymap;
    // "строка 3: ...", показываются в строке состояния при старте
    std::vector<std::string> warnings;
};

}  // namespace ss::app
