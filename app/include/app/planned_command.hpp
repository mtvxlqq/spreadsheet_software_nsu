#pragma once

#include "app/edit_command.hpp"
#include "core/document_edit.hpp"

namespace ss::app {

// Основа для правок с отменой (паттерн Template Method). Наследник только
// считает, что поменять, а применяет и отменяет правку этот класс. Новая
// операция с отменой это наследник с одним методом plan.
class PlannedCommand : public EditCommand {
public:
    // повтор после отмены снова зовёт plan: таблица та же, и правка выйдет та же
    void apply(ss::core::Spreadsheet& document) final;
    void revert(ss::core::Spreadsheet& document) final;

protected:
    PlannedCommand() = default;

    // таблицу не меняет, только читает
    virtual ss::core::DocumentEdit plan(const ss::core::Spreadsheet& document) const = 0;

private:
    ss::core::DocumentEdit edit_;
};

}  // namespace ss::app
