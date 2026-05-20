cmake_minimum_required(VERSION 3.16)

project(CrosshairOverlay LANGUAGES CXX)

add_executable(crosshair WIN32
    src/main.cpp
)

target_compile_features(crosshair PRIVATE cxx_std_17)

if (MSVC)
    target_compile_options(crosshair PRIVATE /W4 /permissive-)
else()
    target_compile_options(crosshair PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(crosshair PRIVATE user32 gdi32 shell32 comctl32 wininet)
