option(SS_SANITIZE "Собрать с AddressSanitizer и UndefinedBehaviorSanitizer" OFF)

if(SS_SANITIZE)
    # глобально, чтобы gtest тоже собирался с санитайзерами
    add_compile_options(
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fno-sanitize-recover=all
    )
    add_link_options(-fsanitize=address,undefined)
endif()
