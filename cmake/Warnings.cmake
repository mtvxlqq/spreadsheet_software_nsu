# Предупреждения включаются только для своих целей: чужой код (GoogleTest)
# не обязан собираться с нашими флагами.
function(ss_set_warnings target)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Werror
    )
endfunction()
