#include "actions/style_actions.hpp"

#include <memory>
#include <optional>
#include <string>

#include "actions/number_argument.hpp"
#include "app/errors.hpp"
#include "app/session.hpp"
#include "app/set_style_command.hpp"
#include "core/cell_style.hpp"
#include "core/number_format_registry.hpp"

namespace ss::app {

namespace {

using ss::core::Align;
using ss::core::CellStyle;

// сколько знаков после запятой, если в команде их не указали
constexpr int default_digits = 2;

// то, что хранится в стиле: "" для general, иначе "имя:знаков"
std::string format_spec(const Session& session, std::span<const std::string> args) {
    if (args.empty() || args.size() > 2) {
        throw CommandError("нужно :format имя [знаков], например :format fixed 2");
    }
    if (args[0] == "general") {
        if (args.size() == 2) {
            throw CommandError("у формата general нет числа знаков");
        }
        return "";
    }
    const int digits = args.size() == 2 ? number_argument(args[1]) : default_digits;
    return session.number_formats().make_spec(args[0], digits);
}

// nullopt это auto: числа вправо, остальное влево
std::optional<Align> align_argument(std::span<const std::string> args) {
    if (args.size() == 1 && args[0] == "left") {
        return Align::Left;
    }
    if (args.size() == 1 && args[0] == "right") {
        return Align::Right;
    }
    if (args.size() == 1 && args[0] == "center") {
        return Align::Center;
    }
    if (args.size() == 1 && args[0] == "auto") {
        return std::nullopt;
    }
    throw CommandError("нужно :align left, right, center или auto");
}

}  // namespace

std::string_view FormatAction::name() const {
    return "format";
}

std::string_view FormatAction::description() const {
    return "формат чисел (:format fixed 2, percent 1, general)";
}

void FormatAction::execute(Session& session, std::span<const std::string> args) const {
    const std::string spec = format_spec(session, args);
    session.apply_edit(std::make_unique<SetStyleCommand>(
        session.target_range(), [spec](CellStyle& style) { style.format = spec; }));
}

std::string_view AlignAction::name() const {
    return "align";
}

std::string_view AlignAction::description() const {
    return "выравнивание (:align left, right, center, auto)";
}

void AlignAction::execute(Session& session, std::span<const std::string> args) const {
    const std::optional<Align> align = align_argument(args);
    session.apply_edit(std::make_unique<SetStyleCommand>(
        session.target_range(), [align](CellStyle& style) { style.align = align; }));
}

}  // namespace ss::app
