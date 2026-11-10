#pragma once

#include <string>

#include "app/action.hpp"
#include "core/ref_mapping.hpp"

namespace ss::app {

// Вставить или удалить строки (столбцы), начиная с первой строки выделения.
// Сколько именно, говорит аргумент (":ir 3"), а без него столько, сколько
// выделено. Один класс на все четыре действия, отличаются только kind и axis.
class StructureAction final : public Action {
public:
    StructureAction(std::string name, std::string description, ss::core::StructureEdit::Kind kind,
                    ss::core::Axis axis);

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;

private:
    std::string name_;
    std::string description_;
    ss::core::StructureEdit::Kind kind_;
    ss::core::Axis axis_;
};

}  // namespace ss::app
