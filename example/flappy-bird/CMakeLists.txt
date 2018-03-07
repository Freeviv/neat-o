
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(flappy "flappy.c")
target_link_libraries(flappy ${CURSES_LIBRARIES} neat-o m)
