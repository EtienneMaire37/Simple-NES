cmake_minimum_required(VERSION 3.10)
project(nes_emulator C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -std=c99 -mno-ms-bitfields -O2")

set(SRC_DIR src)
set(BIN_DIR bin)
set(LIB_DIR libs/CSFML/lib/gcc)
set(DLL_DIR libs/CSFML/bin)
set(INCLUDE_DIR libs/CSFML/include)

file(GLOB SOURCES ${SRC_DIR}/main.c)

include_directories(${INCLUDE_DIR})

add_executable(nes-emulator ${SOURCES})

target_link_directories(nes-emulator PRIVATE ${LIB_DIR})
target_link_libraries(nes-emulator
    csfml-graphics
    csfml-window
    csfml-system
    winmm
)

set_target_properties(nes-emulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BIN_DIR}
)

add_custom_command(
    TARGET nes-emulator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/${DLL_DIR} ${CMAKE_BINARY_DIR}/${BIN_DIR}
)