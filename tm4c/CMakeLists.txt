cmake_minimum_required(VERSION 3.16)
project(gpsdo)
find_program(OBJCOPY arm-none-eabi-objcopy)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb -std=c17 -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -Os")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -std=c++17 -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -Os")

set(CMAKE_LINKER arm-none-eabi-ld)
set(LINKER_FLAGS "-T ${CMAKE_SOURCE_DIR}/tm4c1249ncpdt.ld --entry ISR_Reset")
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> ${LINKER_FLAGS} <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> ${LINKER_FLAGS} <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

add_executable(
        gpsdo

        main.c
        hw/interrupts.c
        lib/clk.c
        lib/delay.c
        lib/epd.c
        lib/font.c
        lib/fonts/ascii_16.c
        lib/led.c
)

# split out debug symbols
add_custom_command(
        TARGET gpsdo POST_BUILD
        COMMAND ${OBJCOPY} --only-keep-debug $<TARGET_FILE:gpsdo> $<TARGET_FILE:gpsdo>.debug
        COMMAND ${OBJCOPY} --strip-debug $<TARGET_FILE:gpsdo>
        COMMAND ${OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE_NAME:gpsdo>.debug $<TARGET_FILE:gpsdo>
)

# create file for lm4flash
add_custom_command(
        TARGET gpsdo POST_BUILD
        COMMAND ${OBJCOPY} --output-target binary $<TARGET_FILE:gpsdo> $<TARGET_FILE:gpsdo>.bin
)
