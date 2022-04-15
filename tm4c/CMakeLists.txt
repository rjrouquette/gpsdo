cmake_minimum_required(VERSION 3.21)
project(tm4c C)


set(CMAKE_C_STANDARD 17)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -Os")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -Os")


add_executable(
        tm4c

        main.c
        lib/interrupts.c
)

add_link_options(tm4c -T tm4c1249ncpdt.ld)
