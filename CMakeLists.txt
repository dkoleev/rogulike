cmake_minimum_required(VERSION 3.24)
project(roguelike)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(EnTT
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.14.0
        GIT_SHALLOW TRUE)
FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.7.1
        GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(EnTT Catch2)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS src/core/*.cpp)
add_library(game_core STATIC ${CORE_SOURCES})
target_include_directories(game_core PUBLIC src)
target_link_libraries(game_core PUBLIC EnTT::EnTT)

file(GLOB UI_SOURCES CONFIGURE_DEPENDS src/ui/*.cpp)
list(FILTER UI_SOURCES EXCLUDE REGEX ".*terminal_(win|posix)\\.cpp$")
add_library(game_ui STATIC ${UI_SOURCES})
target_link_libraries(game_ui PUBLIC game_core)
if(WIN32)
    target_sources(game_ui PRIVATE src/ui/terminal_win.cpp)
else()
    target_sources(game_ui PRIVATE src/ui/terminal_posix.cpp)
endif()

add_executable(roguelike src/main.cpp)
target_link_libraries(roguelike PRIVATE game_ui)


enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
add_executable(game_tests ${TEST_SOURCES})
target_include_directories(game_tests PRIVATE tests)
target_link_libraries(game_tests PRIVATE game_core game_ui Catch2::Catch2WithMain)
add_test(NAME game_tests COMMAND game_tests)
