#pragma once

#include "app/action.hpp"

namespace ss::app {

// правка ячейки под курсором, в строке сразу то, что там было
class StartEditAction final : public Action {
public:
    StartEditAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

class ClearCellAction final : public Action {
public:
    ClearCellAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

class UndoAction final : public Action {
public:
    UndoAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

class RedoAction final : public Action {
public:
    RedoAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

}  // namespace ss::app
