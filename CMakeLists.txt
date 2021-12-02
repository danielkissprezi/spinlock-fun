cmake_minimum_required(VERSION 3.21)
project('deadly_spin' CXX)

find_package(Threads REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(deadly_spin main.cpp)
set_property(TARGET deadly_spin PROPERTY CXX_STANDARD 17)
target_link_libraries(deadly_spin PRIVATE Threads::Threads)
