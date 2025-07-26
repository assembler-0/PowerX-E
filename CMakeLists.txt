cmake_minimum_required(VERSION 3.30)
project(PowerX-E LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=native -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -fsanitize=address")

set(SOURCES
    src/main.c
        src/memory.c
    src/instruction.c
)

add_executable(pwrxe ${SOURCES})
target_include_directories(pwrxe PRIVATE src)