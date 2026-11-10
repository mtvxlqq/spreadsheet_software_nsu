#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "app/edit_command.hpp"

namespace ss::core {
class Spreadsheet;
}

namespace ss::app {

// Сделанные правки для отмены и отменённые для повтора. Самые старые
// правки забываются, чтобы память не росла без конца.
class UndoStack final {
public:
    explicit UndoStack(std::size_t depth = 256);

    // команда уже применена; новая правка делает повтор невозможным
    void push(std::unique_ptr<EditCommand> command) noexcept;
    // false, если отменять нечего; если revert бросит, стек не меняется
    bool undo(ss::core::Spreadsheet& document);
    bool redo(ss::core::Spreadsheet& document);
    void clear() noexcept;

    bool can_undo() const {
        return !done_.empty();
    }

    bool can_redo() const {
        return !undone_.empty();
    }

private:
    std::size_t depth_;
    // последняя сделанная правка в конце
    std::vector<std::unique_ptr<EditCommand>> done_;
    std::vector<std::unique_ptr<EditCommand>> undone_;
};

}  // namespace ss::app
