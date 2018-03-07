
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(neat-o_test "test.c" "greatest.h")
target_link_libraries(neat-o_test neat-o)

