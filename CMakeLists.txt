cmake_minimum_required(VERSION 3.15)
project(simple_nes C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -mno-ms-bitfields -O2 -fno-strict-aliasing")

include(FetchContent)
FetchContent_Declare(CSFML
    GIT_REPOSITORY https://github.com/SFML/CSFML.git
    GIT_TAG 2.6.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(CSFML)

set(SRC_DIR src)
set(BIN_DIR bin)

set(CMAKE_C_COMPILER gcc)

file(GLOB SOURCES ${SRC_DIR}/main.c ${SRC_DIR}/nes.c ${SRC_DIR}/ppu.c ${SRC_DIR}/rp_2a03_apu.c ${SRC_DIR}/rp_2a03_cpu.c ${SRC_DIR}/emulation.c)

add_executable(simple-nes ${SOURCES})

target_link_libraries(simple-nes csfml-graphics csfml-window csfml-system)

set_target_properties(simple-nes PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BIN_DIR}
)
