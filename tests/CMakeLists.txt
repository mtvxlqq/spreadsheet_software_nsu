include(FetchContent)
include(GoogleTest)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(core_tests
    core/test_address.cpp
    core/test_cell_input.cpp
    core/test_csv.cpp
    core/test_error_literal.cpp
    core/test_evaluator.cpp
    core/test_extensions.cpp
    core/test_format_registry.cpp
    core/test_functions.cpp
    core/test_graph.cpp
    core/test_lexer.cpp
    core/test_native_format.cpp
    core/test_parser.cpp
    core/test_printer.cpp
    core/test_recalculator.cpp
    core/test_ref_collector.cpp
    core/test_sheet.cpp
    core/test_spreadsheet.cpp
    core/test_value.cpp
)
target_include_directories(core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# файлы для тестов лежат в testdata/ в корне репозитория
target_compile_definitions(core_tests PRIVATE SS_TESTDATA_DIR="${PROJECT_SOURCE_DIR}/testdata")
target_link_libraries(core_tests PRIVATE ss_core GTest::gtest_main)
ss_set_warnings(core_tests)
gtest_discover_tests(core_tests)

add_executable(app_tests
    app/test_actions.cpp
    app/test_edit_command.cpp
    app/test_extensions.cpp
    app/test_keymap.cpp
    app/test_line_editor.cpp
    app/test_session_keys.cpp
    app/test_task_runner.cpp
    app/test_viewport.cpp
)
target_include_directories(app_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(app_tests PRIVATE SS_TESTDATA_DIR="${PROJECT_SOURCE_DIR}/testdata")
target_link_libraries(app_tests PRIVATE ss_app GTest::gtest_main)
ss_set_warnings(app_tests)
gtest_discover_tests(app_tests)

add_executable(tui_tests
    tui/test_key_names.cpp
    tui/test_screen_layout.cpp
    tui/test_screen_view.cpp
    tui/test_text_fit.cpp
)
target_include_directories(tui_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tui_tests PRIVATE ss_tui GTest::gtest_main)
ss_set_warnings(tui_tests)
gtest_discover_tests(tui_tests)
