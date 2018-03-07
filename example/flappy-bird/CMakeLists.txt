
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(flappy "flappy.c")
set_property(TARGET flappy PROPERTY C_STANDARD 99)

target_link_libraries(flappy ${CURSES_LIBRARIES} neat-o m)
