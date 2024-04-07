cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

project(gpsdo)
find_program(OBJCOPY arm-none-eabi-objcopy)
find_program(OBJDUMP arm-none-eabi-objdump)

# C/C++ 2017 Standard
#set(CMAKE_C_STANDARD 17)
#set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# ARM options
add_compile_options(-mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
# no operating system
add_compile_options(-fno-exceptions -specs=nano.specs -specs=nosys.specs)
# warnings
add_compile_options(-Wall -Wreturn-type)
# debug options
add_compile_options(-ggdb -Os)

# ARM options
add_link_options(-mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
# no operating system
add_link_options(-fno-exceptions -specs=nano.specs -specs=nosys.specs)
# target linker definitions
add_link_options(-Wl,-T${CMAKE_SOURCE_DIR}/hw/linker/tm4c1249ncpdt.ld)
# entry on reset interrupt vector
add_link_options(-Wl,--entry,ISR_Reset)
# prune unused code blocks
add_link_options(-Wl,--gc-sections)

# Include generated headers.
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(
        gpsdo

        main.cpp
        lib/snmp/snmp.c
        lib/snmp/snmp.h
        status.cpp
        status.hpp

        # hardware target files
        hw/adc.h
        hw/crc.h
        hw/eeprom.c
        hw/eeprom.h
        hw/emac.c
        hw/emac.h
        hw/gpio.h
        hw/i2c.h
        hw/interrupts.c
        hw/interrupts.h
        hw/register.h
        hw/ssi.h
        hw/sys.c
        hw/sys.h
        hw/timer.h
        hw/uart.h

        # chrony header files
        lib/chrony/addressing.h
        lib/chrony/candm.h

        # timekeeping
        lib/clk/clk.c
        lib/clk/clk.h
        lib/clk/comp.c
        lib/clk/comp.h
        lib/clk/mono.c
        lib/clk/mono.h
        lib/clk/tai.c
        lib/clk/tai.h
        lib/clk/util.c
        lib/clk/util.h

        lib/delay.c
        lib/delay.h
        lib/format.c
        lib/format.h
        lib/gps.c
        lib/gps.h
        lib/led.c
        lib/led.h

        # networking
        lib/net.cpp
        lib/net.h
        lib/net/arp.c
        lib/net/arp.h
        lib/net/dhcp.c
        lib/net/dhcp.h
        lib/net/dns.c
        lib/net/dns.h
        lib/net/eth.h
        lib/net/icmp.c
        lib/net/icmp.h
        lib/net/ip.c
        lib/net/ip.h
        lib/net/udp.c
        lib/net/udp.h
        lib/net/util.c
        lib/net/util.h

        # NTP server
        lib/ntp/common.hpp
        lib/ntp/gps.cpp
        lib/ntp/gps.hpp
        lib/ntp/ntp.cpp
        lib/ntp/ntp.hpp
        lib/ntp/peer.cpp
        lib/ntp/peer.hpp
        lib/ntp/pll.c
        lib/ntp/pll.h
        lib/ntp/source.cpp
        lib/ntp/source.hpp
        lib/ntp/tcmp.cpp
        lib/ntp/tcmp.h
        lib/chrony/util.cpp
        lib/chrony/util.hpp

        # PTP server
        lib/ptp/common.cpp
        lib/ptp/common.hpp
        lib/ptp/ptp.cpp
        lib/ptp/ptp.hpp

        lib/rand.c
        lib/rand.h
        lib/run.cpp
        lib/run.h

        lib/snmp/sensors.c
        lib/snmp/sensors.h
        lib/snmp/util.c
        lib/snmp/util.h
)

target_link_libraries(
        gpsdo
        -static
)

# a custom target that is always built
add_custom_target(githeader ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/githeader.h)

# creates githeader.h using cmake script
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/githeader.h
        COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/getgit.cmake)

# gitversion.h is a generated file
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/gitversion.h
        PROPERTIES GENERATED TRUE
        HEADER_FILE_ONLY TRUE)

add_dependencies(gpsdo githeader)

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
