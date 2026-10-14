cmake_minimum_required(VERSION 4.3)
project(SmartPtr)

set(CMAKE_CXX_STANDARD 20)

add_executable(SmartPtr main.cpp)

# Функциональные тесты: санитайзеры ловят двойное удаление и обращение к удалённой памяти
add_executable(SmartPtr_tests
        tests/test.cpp
        tests/testUnqPtr.cpp
        tests/testShrdPtr.cpp
        tests/Tracked.h
        core/UnqPtr.h
        core/ShrdPtr.h
)

target_compile_options(SmartPtr_tests PRIVATE -fsanitize=address,undefined -g -O1)
target_link_options(SmartPtr_tests PRIVATE -fsanitize=address,undefined)
