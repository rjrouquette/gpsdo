cmake_minimum_required(VERSION 3.16)
project(gpsdo)
find_program(OBJCOPY arm-none-eabi-objcopy)
find_program(OBJDUMP arm-none-eabi-objdump)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

set(COMP_FLAGS "-ggdb -Os -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS "-std=c17 ${COMP_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=c++17 ${COMP_FLAGS}")

set(CMAKE_LINKER arm-none-eabi-ld)
set(LINKER_FLAGS "-T ${CMAKE_SOURCE_DIR}/tm4c1249ncpdt.ld --entry ISR_Reset --gc-sections")
set(LINKER_FLAGS "${LINKER_FLAGS} -L/usr/lib/arm-none-eabi/lib/thumb/v7e-m+fp/hard")
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> ${LINKER_FLAGS} <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> ${LINKER_FLAGS} <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

add_executable(
        gpsdo

        main.c
        ntp.c
        snmp.c
        hw/emac.c
        hw/interrupts.c
        lib/clk.c
        lib/delay.c
        lib/epd.c
        lib/font.c
        lib/fonts/ascii_16.c
        lib/format.c
        lib/led.c
        lib/plot.c
        lib/net.c
        lib/net/arp.c
        lib/net/dhcp.c
        lib/net/icmp.c
        lib/net/ip.c
        lib/net/udp.c
        lib/net/util.c
        lib/temp.c
)

target_link_libraries(
        gpsdo
        -static
        -lc
        -lm
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
add_custom_command(
        TARGET gpsdo POST_BUILD
        COMMAND ${OBJDUMP} -h -S $<TARGET_FILE:gpsdo> > $<TARGET_FILE:gpsdo>.lst
)
