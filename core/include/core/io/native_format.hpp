#pragma once

#include "core/io/file_reader.hpp"
#include "core/io/file_writer.hpp"

namespace ss::core {

// Наш формат .sst. Первая строка "SST 2", дальше по строке на ячейку:
// адрес, таб и то, что ввёл пользователь. В тексте \ пишется как \\,
// таб как \t, перевод строки как \n. Пустые строки пропускаются.
//
// После ячеек идёт оформление, такие строки начинаются с '!':
//   !style  A1  fixed:2  right    формат чисел и выравнивание ячейки
//   !width  B   15                ширина столбца
// Адрес с '!' начинаться не может, поэтому их не спутать с ячейками.
//
// Пишем всегда версию 2, а читаем ещё и старую "SST 1", где оформления нет.
class NativeFormat final : public FileReader, public FileWriter {
public:
    NativeFormat() = default;

    // одна реализация закрывает name() и extensions() обеих баз
    std::string_view name() const override;
    std::vector<std::string> extensions() const override;

    void load(std::istream& in, CellSink& sink) const override;
    void save(std::ostream& out, const CellSource& source) const override;
};

}  // namespace ss::core
