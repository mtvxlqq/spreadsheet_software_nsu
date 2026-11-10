#pragma once

#include "app/action.hpp"

namespace ss::app {

// начать выделение с ячейки под курсором
class StartSelectionAction final : public Action {
public:
    StartSelectionAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;

    // иначе выделение, начатое заново, тут же снялось бы
    bool keeps_selection() const override {
        return true;
    }
};

// копирует выделенный диапазон или ячейку под курсором
class CopyAction final : public Action {
public:
    CopyAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

class PasteAction final : public Action {
public:
    PasteAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

}  // namespace ss::app
