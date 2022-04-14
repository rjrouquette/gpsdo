cmake_minimum_required(VERSION 3.21)
project(tm4c C)


set(CMAKE_C_STANDARD 17)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

add_executable(
        tm4c

        main.c
        lib/interrupts.c
        lib/sections.c
)
