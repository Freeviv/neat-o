
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(neat-o_test "test.c" "greatest.h")
set_property(TARGET neat-o_test PROPERTY C_STANDARD 99)
target_link_libraries(neat-o_test neat-o)

