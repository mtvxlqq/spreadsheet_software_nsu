#include <memory>

#include <gtest/gtest.h>

#include "core/errors.hpp"
#include "core/io/csv_reader.hpp"
#include "core/io/format_registry.hpp"
#include "core/io/native_format.hpp"
#include "core/io/register_formats.hpp"

using ss::core::FileFormatError;
using ss::core::FormatRegistry;

namespace {

class FormatRegistryTest : public ::testing::Test {
protected:
    FormatRegistryTest() {
        ss::core::register_formats(registry);
    }

    FormatRegistry registry;
};

}  // namespace

TEST_F(FormatRegistryTest, ChoosesByExtension) {
    EXPECT_EQ(registry.reader_for("table.sst").name(), "SST");
    EXPECT_EQ(registry.writer_for("table.sst").name(), "SST");
    EXPECT_EQ(registry.reader_for("data.csv").name(), "CSV");
}

TEST_F(FormatRegistryTest, ExtensionIgnoresCaseAndDirectories) {
    EXPECT_EQ(registry.reader_for("TABLE.SST").name(), "SST");
    EXPECT_EQ(registry.reader_for("dir.csv/data.Csv").name(), "CSV");
    EXPECT_EQ(registry.reader_for("/home/user/my.table.sst").name(), "SST");
}

TEST_F(FormatRegistryTest, UnknownFormat) {
    EXPECT_THROW(static_cast<void>(registry.reader_for("table.xlsx")), FileFormatError);
    EXPECT_THROW(static_cast<void>(registry.reader_for("table")), FileFormatError);
    EXPECT_THROW(static_cast<void>(registry.writer_for("table.xlsx")), FileFormatError);
}

TEST_F(FormatRegistryTest, DuplicateExtensionIsRejected) {
    EXPECT_THROW(registry.add_reader(std::make_shared<const ss::core::CsvReader>()),
                 ss::core::Error);
    // старый CSV на месте
    EXPECT_EQ(registry.reader_for("data.csv").name(), "CSV");
}

// в CSV можно и писать, но только как экспорт: формулы он не хранит
TEST_F(FormatRegistryTest, CsvWriterIsForExportOnly) {
    EXPECT_EQ(registry.writer_for("data.csv").name(), "CSV");
    EXPECT_FALSE(registry.writer_for("data.CSV").keeps_formulas());
    EXPECT_TRUE(registry.writer_for("table.sst").keeps_formulas());
}
