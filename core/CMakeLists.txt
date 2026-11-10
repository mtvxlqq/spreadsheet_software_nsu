add_library(ss_core STATIC
    src/address.cpp
    src/builtins/average_function.cpp
    src/builtins/if_function.cpp
    src/builtins/max_function.cpp
    src/builtins/min_function.cpp
    src/builtins/number_args.cpp
    src/builtins/register_builtins.cpp
    src/builtins/sum_function.cpp
    src/cell_input.cpp
    src/dependency_graph.cpp
    src/document_edit.cpp
    src/evaluator.cpp
    src/expr.cpp
    src/formula_printer.cpp
    src/formula_rewrite.cpp
    src/function_registry.cpp
    src/io/csv_reader.cpp
    src/io/format_registry.cpp
    src/io/native_format.cpp
    src/io/register_formats.cpp
    src/lexer.cpp
    src/number_format.cpp
    src/number_format_registry.cpp
    src/parser.cpp
    src/recalculator.cpp
    src/ref_collector.cpp
    src/ref_mapping.cpp
    src/ref_shifter.cpp
    src/sheet.cpp
    src/sheet_eval_context.cpp
    src/spreadsheet.cpp
    src/style_table.cpp
    src/text.cpp
    src/value.cpp
)
target_include_directories(ss_core PUBLIC include)
# заголовки встроенных функций снаружи не нужны
target_include_directories(ss_core PRIVATE src)
ss_set_warnings(ss_core)
