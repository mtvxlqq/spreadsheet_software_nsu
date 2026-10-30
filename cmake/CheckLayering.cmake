# Проверка слоёв: core и app не должны зависеть от FTXUI.
# Смотрим и на библиотеки (вместе с транзитивными), и на #include в исходниках.

# собирает все библиотеки цели, включая транзитивные
function(_ss_collect_link_libraries target out_var)
    set(visited ${${out_var}})
    if(target IN_LIST visited)
        return()
    endif()
    list(APPEND visited ${target})

    if(TARGET ${target})
        get_target_property(libs ${target} LINK_LIBRARIES)
        get_target_property(interface_libs ${target} INTERFACE_LINK_LIBRARIES)
        foreach(lib IN LISTS libs interface_libs)
            if(lib AND NOT lib MATCHES "-NOTFOUND$")
                _ss_collect_link_libraries(${lib} visited)
            endif()
        endforeach()
    endif()

    set(${out_var} ${visited} PARENT_SCOPE)
endfunction()

function(_ss_check_no_tui_includes target)
    get_target_property(source_dir ${target} SOURCE_DIR)
    file(GLOB_RECURSE files "${source_dir}/*.hpp" "${source_dir}/*.cpp")
    foreach(file IN LISTS files)
        file(STRINGS ${file} tui_includes REGEX "#[ \t]*include[ \t]*[<\"]ftxui/")
        if(tui_includes)
            message(FATAL_ERROR
                "Нарушение слоёв: ${file} подключает заголовки FTXUI")
        endif()
    endforeach()
endfunction()

# вызывать после объявления всех целей; тех, что ещё не созданы, пропускаем
function(ss_check_layering)
    foreach(target IN LISTS ARGN)
        if(NOT TARGET ${target})
            continue()
        endif()

        set(all_libs "")
        _ss_collect_link_libraries(${target} all_libs)
        foreach(lib IN LISTS all_libs)
            if(lib MATCHES "ftxui")
                message(FATAL_ERROR
                    "Нарушение слоёв: ${target} линкуется с ${lib}")
            endif()
        endforeach()

        _ss_check_no_tui_includes(${target})
    endforeach()
endfunction()
