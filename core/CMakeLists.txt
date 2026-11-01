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
    src/evaluator.cpp
    src/expr.cpp
    src/formula_printer.cpp
    src/function_registry.cpp
    src/lexer.cpp
    src/number_format.cpp
    src/parser.cpp
    src/recalculator.cpp
    src/ref_collector.cpp
    src/sheet.cpp
    src/text.cpp
    src/value.cpp
)
target_include_directories(ss_core PUBLIC include)
# заголовки встроенных функций снаружи не нужны
target_include_directories(ss_core PRIVATE src)
ss_set_warnings(ss_core)
