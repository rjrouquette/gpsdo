cmake_minimum_required(VERSION 3.0)

#find toolchain programs
find_program(MSP430-GCC msp430-elf-gcc)
find_program(MSP430-GXX msp430-elf-g++)
find_program(MSP430-OBJCOPY msp430-elf-objcopy)
find_program(MSP430-SIZE msp430-elf-size)
find_program(MSP430-OBJDUMP msp430-elf-objdump)
#find_program(MSPDEBUG mspdebug)

get_filename_component(MSP430_INCLUDE "${MSP430-GCC}" DIRECTORY)
get_filename_component(MSP430_INCLUDE "${MSP430_INCLUDE}/../include" ABSOLUTE)
include_directories("${MSP430_INCLUDE}")
link_directories("${MSP430_INCLUDE}")

#define toolchain
set(CMAKE_SYSTEM_NAME Generic)
# Use msp430-gcc and msp430-g++
set(CMAKE_C_COMPILER ${MSP430-GCC})
set(CMAKE_CXX_COMPILER ${MSP430-GXX})

# don't test since they are cross-compiling
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

project("gpsdo")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -Wall -mmcu=MSP430F5171 -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--relax,--gc-sections")

add_executable(
        gpsdo

        main.c
        math.c
        math.h
        pid.c
        pid.h
        pps.c
        pps.h
        tcxo.c
        tcxo.h
)
