#pragma once

#include "app/mode.hpp"

namespace ss::app {

// Клавиша ищется в Keymap, и запускается её действие. Печатный символ
// без привязки начинает правку ячейки с этого символа.
class NormalMode final : public Mode {
public:
    NormalMode() = default;

    std::string_view name() const override;
    void handle_key(Session& session, std::string_view key) const override;
};

}  // namespace ss::app
