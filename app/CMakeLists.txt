add_library(ss_app STATIC
    src/action_registry.cpp
    src/actions/edit_actions.cpp
    src/actions/file_actions.cpp
    src/actions/mode_actions.cpp
    src/actions/navigation_actions.cpp
    src/actions/quit_actions.cpp
    src/cell_presenter.cpp
    src/document_io.cpp
    src/keymap.cpp
    src/line_editor.cpp
    src/modes/command_mode.cpp
    src/modes/edit_mode.cpp
    src/modes/help_mode.cpp
    src/modes/normal_mode.cpp
    src/register_actions.cpp
    src/session.cpp
    src/set_cell_command.cpp
    src/task_runner.cpp
    src/viewport.cpp
)
target_include_directories(ss_app PUBLIC include)
# классы конкретных действий снаружи не нужны
target_include_directories(ss_app PRIVATE src)
find_package(Threads REQUIRED)
target_link_libraries(ss_app PUBLIC ss_core Threads::Threads)
ss_set_warnings(ss_app)
