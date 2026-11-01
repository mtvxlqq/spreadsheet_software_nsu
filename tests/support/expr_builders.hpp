#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/address.hpp"
#include "core/expr.hpp"

// Чтобы собирать деревья формул в тестах руками, без парсера.
namespace ss::test {

using core::ExprPtr;

inline ExprPtr num(double value) {
    return std::make_unique<core::NumberLit>(value);
}

inline ExprPtr str(std::string text) {
    return std::make_unique<core::StringLit>(std::move(text));
}

inline ExprPtr cell(std::string_view ref) {
    return std::make_unique<core::RefExpr>(core::CellRef::try_parse(ref).value());
}

inline ExprPtr range_of(std::string_view from, std::string_view to) {
    return std::make_unique<core::RangeExpr>(core::CellRef::try_parse(from).value(),
                                             core::CellRef::try_parse(to).value());
}

inline ExprPtr neg(ExprPtr operand) {
    return std::make_unique<core::UnaryExpr>(core::UnaryOp::Neg, std::move(operand));
}

inline ExprPtr unary_plus(ExprPtr operand) {
    return std::make_unique<core::UnaryExpr>(core::UnaryOp::Plus, std::move(operand));
}

inline ExprPtr bin(core::BinaryOp op, ExprPtr left, ExprPtr right) {
    return std::make_unique<core::BinaryExpr>(op, std::move(left), std::move(right));
}

// call("SUM", cell("A1"), num(2)) - вызов без настоящей функции (nullptr),
// для печати и поиска ссылок она не нужна
template <typename... Args>
ExprPtr call(std::string name, Args... args) {
    std::vector<ExprPtr> list;
    (list.push_back(std::move(args)), ...);
    return std::make_unique<core::FunctionCallExpr>(std::move(name), std::move(list), nullptr);
}

}  // namespace ss::test
