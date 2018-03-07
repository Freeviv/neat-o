
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses)

add_executable(flappy "flappy.c")
target_link_libraries(flappy ${CURSES_LIBRARIES} neat-o m)
