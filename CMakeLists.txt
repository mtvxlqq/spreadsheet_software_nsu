cmake_minimum_required(VERSION 3.24)
project(spreadsheet VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(Warnings)
# Флаги санитайзеров добавляются глобально, поэтому модуль подключается
# до объявления любых целей (включая GoogleTest).
include(Sanitizers)
include(CheckLayering)

enable_testing()

add_subdirectory(core)
add_subdirectory(tests)

ss_check_layering(ss_core ss_app)
