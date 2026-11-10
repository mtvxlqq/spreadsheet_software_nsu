#include "app/undo_stack.hpp"

#include <algorithm>
#include <utility>

namespace ss::app {

// Память под оба стека берём сразу. Тогда push и перекладывание команды
// из одного стека в другой не выделяют память и не бросают.
UndoStack::UndoStack(std::size_t depth) : depth_(std::max<std::size_t>(depth, 1)) {
    done_.reserve(depth_);
    undone_.reserve(depth_);
}

void UndoStack::push(std::unique_ptr<EditCommand> command) noexcept {
    undone_.clear();
    if (done_.size() == depth_) {
        done_.erase(done_.begin());
    }
    done_.push_back(std::move(command));
}

bool UndoStack::undo(ss::core::Spreadsheet& document) {
    if (done_.empty()) {
        return false;
    }
    done_.back()->revert(document);
    undone_.push_back(std::move(done_.back()));
    done_.pop_back();
    return true;
}

bool UndoStack::redo(ss::core::Spreadsheet& document) {
    if (undone_.empty()) {
        return false;
    }
    undone_.back()->apply(document);
    done_.push_back(std::move(undone_.back()));
    undone_.pop_back();
    return true;
}

void UndoStack::clear() noexcept {
    done_.clear();
    undone_.clear();
}

}  // namespace ss::app
