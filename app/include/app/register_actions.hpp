#pragma once

namespace ss::app {

class ActionRegistry;

// Регистрирует все встроенные действия. Новое действие: свой класс
// в actions/ и строка здесь.
void register_actions(ActionRegistry& registry);

}  // namespace ss::app
