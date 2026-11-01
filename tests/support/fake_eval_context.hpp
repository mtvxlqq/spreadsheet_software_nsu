#pragma once

#include <map>
#include <string_view>
#include <utility>

#include "core/address.hpp"
#include "core/eval_context.hpp"
#include "core/evaluator.hpp"
#include "core/value.hpp"

namespace ss::test {

// Лист из словаря адрес -> значение. Вычислителю больше ничего и не нужно,
// поэтому его можно проверять без настоящей таблицы.
class FakeEvalContext final : public core::EvalContext {
public:
    void set(std::string_view address, core::ValuePtr value) {
        values_[core::CellAddress::parse(address)] = std::move(value);
    }

    core::ValuePtr value_at(core::CellAddress address) const override {
        const auto it = values_.find(address);
        return it == values_.end() ? core::empty_value() : it->second;
    }

    core::ValuePtr evaluate(const core::Expr& expr) override {
        return core::Evaluator::evaluate(expr, *this);
    }

    core::ArgValues evaluate_flat(const core::Expr& expr) override {
        return core::Evaluator::evaluate_flat(expr, *this);
    }

private:
    std::map<core::CellAddress, core::ValuePtr> values_;
};

}  // namespace ss::test
