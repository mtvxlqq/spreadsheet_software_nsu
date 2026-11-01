add_library(ss_core STATIC
    src/address.cpp
    src/expr.cpp
    src/formula_printer.cpp
    src/lexer.cpp
    src/number_format.cpp
    src/ref_collector.cpp
    src/text.cpp
    src/value.cpp
)
target_include_directories(ss_core PUBLIC include)
ss_set_warnings(ss_core)
