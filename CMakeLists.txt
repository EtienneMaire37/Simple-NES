cmake_minimum_required(VERSION 3.10)
project(simple_nes C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -mno-ms-bitfields -O2 -fno-strict-aliasing")

set(SRC_DIR src)
set(BIN_DIR bin)
set(LIB_DIR ./libs/CSFML/lib/gcc)
set(DLL_DIR ./libs/CSFML/bin)
set(INCLUDE_DIR ./libs/CSFML/include)

set(CMAKE_C_COMPILER gcc)

file(GLOB SOURCES ${SRC_DIR}/main.c)

include_directories(${INCLUDE_DIR})

add_executable(simple-nes ${SOURCES})

target_link_directories(simple-nes PRIVATE ${LIB_DIR})

if (WIN32)
    target_link_libraries(  simple-nes
                            csfml-graphics
                            csfml-window
                            csfml-system
                            winmm
                        )
else()
    target_link_libraries(simple-nes
    csfml-graphics
    csfml-window
    csfml-system
)
endif()

set_target_properties(simple-nes PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${BIN_DIR}
)

if (WIN32)
    add_custom_command(
        TARGET simple-nes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/${DLL_DIR} ${CMAKE_BINARY_DIR}/${BIN_DIR}
    )
endif()
