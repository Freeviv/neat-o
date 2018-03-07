cmake_minimum_required (VERSION 2.8)

option(with_example "build the example" TRUE)
option(with_test "build tests" TRUE)

project(neat-o)

include_directories("include")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(SRCS
    src/neat/genome.c
    src/neat/population.c
    src/neat/species.c
    src/nn/nn.c
)

set(HDRS
    src/neat/genome.h
    src/neat/population.h
    src/neat/species.h
)

add_library(${PROJECT_NAME} ${SRCS} ${HDRS})
target_link_libraries(${PROJECT_NAME} m)

if(${with_example})
    add_subdirectory(example)
endif()

if(${with_test})
    add_subdirectory(test)
endif()

