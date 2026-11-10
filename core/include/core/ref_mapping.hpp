#pragma once

#include <optional>
#include <utility>

#include "core/address.hpp"

namespace ss::core {

// Как двигать ссылки при правке (паттерн Strategy). RefShifter обходит
// дерево, а что станет с каждой ссылкой, решает этот объект.
class RefMapping {
public:
    virtual ~RefMapping() = default;
    RefMapping(const RefMapping&) = delete;
    RefMapping& operator=(const RefMapping&) = delete;
    RefMapping(RefMapping&&) = delete;
    RefMapping& operator=(RefMapping&&) = delete;

    // nullopt значит, что ссылки больше нет и в формуле будет #REF!
    virtual std::optional<CellRef> map_ref(CellRef ref) const = 0;
    virtual std::optional<std::pair<CellRef, CellRef>> map_range(CellRef from,
                                                                 CellRef to) const = 0;

protected:
    RefMapping() = default;
};

// Формулу скопировали в другое место: относительные части ссылки едут
// вместе с ней, а те, что с $, остаются.
class CopyMapping final : public RefMapping {
public:
    CopyMapping(int d_row, int d_col) : d_row_(d_row), d_col_(d_col) {}

    std::optional<CellRef> map_ref(CellRef ref) const override;
    std::optional<std::pair<CellRef, CellRef>> map_range(CellRef from, CellRef to) const override;

private:
    int d_row_;
    int d_col_;
};

enum class Axis { Row, Column };

// Вставка или удаление count строк (столбцов), начиная с номера at.
struct StructureEdit {
    enum class Kind { Insert, Delete };

    Kind kind = Kind::Insert;
    Axis axis = Axis::Row;
    int at = 0;
    int count = 1;
};

// Строки или столбцы сдвинулись: двигаются все ссылки, $ тут ни при чём.
class StructureMapping final : public RefMapping {
public:
    explicit StructureMapping(StructureEdit edit) : edit_(edit) {}

    std::optional<CellRef> map_ref(CellRef ref) const override;
    std::optional<std::pair<CellRef, CellRef>> map_range(CellRef from, CellRef to) const override;

private:
    int position_of(CellAddress address) const;
    void set_position(CellAddress& address, int position) const;
    int limit() const;
    std::optional<std::pair<int, int>> map_span(int low, int high) const;

    StructureEdit edit_;
};

}  // namespace ss::core
