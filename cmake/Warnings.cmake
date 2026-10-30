# только для своих целей: gtest с нашим -Werror может и не собраться
function(ss_set_warnings target)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Werror
    )
endfunction()
