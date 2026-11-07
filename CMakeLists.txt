cmake_minimum_required(VERSION 4.2)
project(chip8emu C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(SDL3 REQUIRED CONFIG)

file(GLOB SOURCES CONFIGURE_DEPENDS src/*.c)
add_executable(chip8emu ${SOURCES})
target_link_libraries(chip8emu PRIVATE SDL3::SDL3)

if(NOT MSVC)
  target_compile_options(chip8emu PRIVATE -Wall -Wextra -Wpedantic)
endif()
