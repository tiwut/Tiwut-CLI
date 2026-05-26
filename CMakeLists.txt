cmake_minimum_required(VERSION 3.12)
project(TiwutCLI CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CURSES_NEED_NCURSES TRUE)
set(CURSES_NEED_WIDE TRUE)
find_package(Curses REQUIRED)


set(SOURCES
    src/main.cpp
    src/globals.cpp
    src/utils.cpp
    src/system_monitor.cpp
    src/menus.cpp
    src/splashscreen.cpp
    src/dashboard.cpp
    src/widgets/stats_widget.cpp
    src/widgets/notepad_widget.cpp
    src/widgets/launcher_widget.cpp
    src/widgets/explorer_widget.cpp
    src/widgets/git_terminal.cpp
    src/widgets/app_store.cpp
)


add_executable(Tiwut-CLI ${SOURCES})


target_include_directories(Tiwut-CLI PRIVATE src ${CURSES_INCLUDE_DIRS} ${CURSES_INCLUDE_DIR} ${CURSES_INCLUDE_PATH})
target_link_libraries(Tiwut-CLI PRIVATE ${CURSES_LIBRARIES})


install(TARGETS Tiwut-CLI DESTINATION bin)
