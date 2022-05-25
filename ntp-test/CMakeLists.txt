cmake_minimum_required(VERSION 3.22)
project(ntp-test)

set(CMAKE_CXX_STANDARD 17)

add_executable(ntp-test main.cpp)
target_link_libraries(ntp-test -pthread)
