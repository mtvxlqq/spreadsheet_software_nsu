#pragma once

#include "app/action.hpp"

namespace ss::app {

// не выходит, если есть несохранённые изменения
class QuitAction final : public Action {
public:
    QuitAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

// выйти, даже если что-то не сохранено
class ForceQuitAction final : public Action {
public:
    ForceQuitAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

}  // namespace ss::app
