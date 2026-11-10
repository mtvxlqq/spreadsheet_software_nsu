#pragma once

namespace ss::core {

class FormatRegistry;

// Регистрирует .sst (чтение и запись) и .csv (импорт и экспорт значений).
void register_formats(FormatRegistry& registry);

}  // namespace ss::core
