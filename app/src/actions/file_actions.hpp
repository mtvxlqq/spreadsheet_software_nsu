#pragma once

#include "app/action.hpp"

namespace ss::app {

// ":w [путь]", без пути пишет в текущий файл
class SaveAction final : public Action {
public:
    SaveAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

// ":e путь", файл становится текущим
class OpenAction final : public Action {
public:
    OpenAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

// ":import путь", у документа после этого нет файла, надо сохранить через :w
class ImportCsvAction final : public Action {
public:
    ImportCsvAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

// ":export путь": записать значения в другой формат, например CSV.
// Текущий файл документа и отметка о несохранённых изменениях не меняются.
class ExportAction final : public Action {
public:
    ExportAction() = default;

    std::string_view name() const override;
    std::string_view description() const override;
    void execute(Session& session, std::span<const std::string> args) const override;
};

}  // namespace ss::app
