cmake_minimum_required(VERSION 3.14)
project(inventory_manager LANGUAGES C CXX)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

include_directories(include)

# C backend compiled as C
add_library(inventory_c STATIC src/inventory.c)
target_include_directories(inventory_c PUBLIC include)

# C++ frontend compiled as C++
add_executable(inventory_manager
    src/InventoryManager.cpp
    src/main.cpp
)

target_include_directories(inventory_manager PRIVATE include)
target_link_libraries(inventory_manager PRIVATE inventory_c)

# Warnings
target_compile_options(inventory_c       PRIVATE -Wall -Wextra)
target_compile_options(inventory_manager PRIVATE -Wall -Wextra)
