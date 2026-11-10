#pragma once

namespace ss::core {
class Spreadsheet;
}

namespace ss::app {

// Одно изменение документа (паттерн Command). Все правки идут через
// Session::apply_edit, а оттуда в UndoStack, поэтому любую можно отменить.
class EditCommand {
public:
    virtual ~EditCommand() = default;
    EditCommand(const EditCommand&) = delete;
    EditCommand& operator=(const EditCommand&) = delete;
    EditCommand(EditCommand&&) = delete;
    EditCommand& operator=(EditCommand&&) = delete;

    // если бросит, документ остался как был
    virtual void apply(ss::core::Spreadsheet& document) = 0;
    // вернуть как было до apply
    virtual void revert(ss::core::Spreadsheet& document) = 0;

protected:
    EditCommand() = default;
};

}  // namespace ss::app
