#pragma once

namespace ss::core {

class FormatRegistry;

// Регистрирует .sst (чтение и запись) и .csv (только чтение).
void register_formats(FormatRegistry& registry);

}  // namespace ss::core
