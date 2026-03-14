cmake_minimum_required(VERSION 3.15)
project(SuperMegaChess CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Find SFML ----
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# ---- Sources ----
set(SOURCES
    src/main.cpp
    src/ChessEngine.cpp
    src/ChessBot.cpp
    src/Gui.cpp
)

add_executable(chess ${SOURCES})

target_include_directories(chess PRIVATE src)

target_link_libraries(chess
    sfml-graphics
    sfml-window
    sfml-system
)

# Enable warnings
if(MSVC)
    target_compile_options(chess PRIVATE /W4)
else()
    target_compile_options(chess PRIVATE -Wall -Wextra -Wpedantic)
endif()
