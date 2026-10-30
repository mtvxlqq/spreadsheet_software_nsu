option(SS_SANITIZE "Собрать с AddressSanitizer и UndefinedBehaviorSanitizer" OFF)

if(SS_SANITIZE)
    # Глобально, а не на цель: и наш код, и GoogleTest должны быть
    # инструментированы одинаково.
    add_compile_options(
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fno-sanitize-recover=all
    )
    add_link_options(-fsanitize=address,undefined)
endif()
