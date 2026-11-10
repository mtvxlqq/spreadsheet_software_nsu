#pragma once

#include "app/action.hpp"

namespace ss::app {

class CommandModeAction final : public Action {
public:
    CommandModeAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;

    // команду набирают уже при выделении, и работает она с ним
    bool keeps_selection() const override {
        return true;
    }
};

class ShowHelpAction final : public Action {
public:
    ShowHelpAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

}  // namespace ss::app
