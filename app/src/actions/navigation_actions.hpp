#pragma once

#include <string>

#include "app/action.hpp"

namespace ss::app {

// Сдвиг курсора на (d_row, d_col). Один класс на все четыре стрелки,
// отличаются только числа.
class MoveAction final : public Action {
public:
    MoveAction(std::string name, std::string description, int d_row, int d_col);

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;

private:
    std::string name_;
    std::string description_;
    int d_row_;
    int d_col_;
};

// на экран вверх (direction = -1) или вниз (direction = 1)
class PageAction final : public Action {
public:
    PageAction(std::string name, std::string description, int direction);

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;

private:
    std::string name_;
    std::string description_;
    int direction_;
};

// в столбец A той же строки
class GoHomeAction final : public Action {
public:
    GoHomeAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

// в A1
class GoTopAction final : public Action {
public:
    GoTopAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

}  // namespace ss::app
