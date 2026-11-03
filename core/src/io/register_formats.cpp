#include "core/io/register_formats.hpp"

#include <memory>

#include "core/io/csv_reader.hpp"
#include "core/io/format_registry.hpp"
#include "core/io/native_format.hpp"

namespace ss::core {

void register_formats(FormatRegistry& registry) {
    // один объект и читает, и пишет
    const auto native = std::make_shared<const NativeFormat>();
    registry.add_reader(native);
    registry.add_writer(native);
    registry.add_reader(std::make_shared<const CsvReader>());
}

}  // namespace ss::core
