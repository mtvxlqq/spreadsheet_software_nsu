#pragma once

#include "core/document_edit.hpp"
#include "core/ref_mapping.hpp"

namespace ss::core {

class FunctionProvider;
class Spreadsheet;

// Считает правку для вставки или удаления строк (столбцов): куда переедут
// ячейки вместе с оформлением, как изменятся формулы во всём листе и ширины
// столбцов. Саму таблицу не трогает, результат применяет Spreadsheet::apply.
//
// Формулы меняются у всех ячеек, а не только у сдвинутых: =A10 в первой
// строке после вставки строки 5 должна стать =A11. Ссылка на удалённую
// ячейку превращается в #REF!.
//
// LimitError, если номер или количество не подходят листу, либо если
// вставка вытолкнула бы заполненную ячейку за его край.
DocumentEdit plan_structure_edit(const Spreadsheet& document, const FunctionProvider& functions,
                                 StructureEdit edit);

}  // namespace ss::core
