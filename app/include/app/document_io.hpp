#pragma once

#include <filesystem>
#include <memory>

namespace ss::core {
class CellSource;
class FileReader;
class FileWriter;
class FunctionProvider;
class Spreadsheet;
}  // namespace ss::core

namespace ss::app {

// Читает файл в новую таблицу. Если файл битый, бросает, а текущий
// документ сеанса никто не трогал. Сеанс тут не нужен, поэтому на
// следующем этапе это можно будет делать в фоновом потоке.
std::unique_ptr<ss::core::Spreadsheet> load_document(const std::filesystem::path& path,
                                                      const ss::core::FileReader& reader,
                                                      const ss::core::FunctionProvider& functions);

// Пишет сначала во временный файл рядом и потом переименовывает его.
// Если запись упала на середине, старый файл останется целым.
void save_document(const std::filesystem::path& path, const ss::core::FileWriter& writer,
                   const ss::core::CellSource& source);

}  // namespace ss::app
