include(FetchContent)

FetchContent_Declare(
    ftxui
    URL https://github.com/ArthurSonzogni/FTXUI/archive/refs/tags/v7.0.3.tar.gz
    URL_HASH SHA256=e7c62ffe19009759821b4f0f8df7f2a6fb83784c3a9f1477d81f56d3ee723c88
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(FTXUI_QUIET ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ftxui)

add_executable(spreadsheet src/main.cpp)
target_link_libraries(spreadsheet PRIVATE ss_app ftxui::component)
ss_set_warnings(spreadsheet)
