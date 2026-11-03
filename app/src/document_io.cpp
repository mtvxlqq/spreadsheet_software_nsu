#include "app/document_io.hpp"

#include <fstream>
#include <string>
#include <system_error>

#include "core/errors.hpp"
#include "core/io/file_reader.hpp"
#include "core/io/file_writer.hpp"
#include "core/spreadsheet.hpp"

namespace ss::app {

std::unique_ptr<ss::core::Spreadsheet> load_document(const std::filesystem::path& path,
                                                      const ss::core::FileReader& reader,
                                                      const ss::core::FunctionProvider& functions) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw ss::core::Error("не удалось открыть " + path.string());
    }
    auto document = std::make_unique<ss::core::Spreadsheet>(functions);
    reader.load(in, *document);
    return document;
}

void save_document(const std::filesystem::path& path, const ss::core::FileWriter& writer,
                   const ss::core::CellSource& source) {
    std::filesystem::path temp = path;
    temp += ".tmp";
    std::error_code ignored;
    {
        std::ofstream out(temp, std::ios::binary);
        if (!out) {
            throw ss::core::Error("не удалось записать " + path.string());
        }
        try {
            writer.save(out, source);
        } catch (...) {
            out.close();
            std::filesystem::remove(temp, ignored);
            throw;
        }
    }
    std::error_code error;
    std::filesystem::rename(temp, path, error);
    if (error) {
        std::filesystem::remove(temp, ignored);
        throw ss::core::Error("не удалось записать " + path.string() + ": " + error.message());
    }
}

}  // namespace ss::app
