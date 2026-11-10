#pragma once

#include "app/action.hpp"

namespace ss::app {

// ":format fixed 2", ":format percent 1", ":format general"
class FormatAction final : public Action {
public:
    FormatAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

// ":align left", "right", "center" или "auto"
class AlignAction final : public Action {
public:
    AlignAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

}  // namespace ss::app
