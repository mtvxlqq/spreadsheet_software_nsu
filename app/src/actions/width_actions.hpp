#pragma once

#include <string>

#include "app/action.hpp"

namespace ss::app {

// Столбец под курсором на delta шире. Один класс на "шире" и "уже",
// отличается только знак.
class ColumnWidthStepAction final : public Action {
public:
    ColumnWidthStepAction(std::string name, std::string description, int delta);

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;

    // чтобы подбирать ширину выделенных столбцов несколькими нажатиями
    bool keeps_selection() const override {
        return true;
    }

private:
    std::string name_;
    std::string description_;
    int delta_;
};

// ":width 15"
class WidthAction final : public Action {
public:
    WidthAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

}  // namespace ss::app
