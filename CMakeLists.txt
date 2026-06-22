cmake_minimum_required(VERSION 3.20)

project(cpp_user_runtime
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(runtime
    src/task.cpp
    src/runtime.cpp
    src/scheduler.cpp
)

target_include_directories(runtime
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_options(runtime
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_executable(hello_runtime
    examples/hello_runtime.cpp
)

target_link_libraries(hello_runtime
    PRIVATE
        runtime
)

enable_testing()
add_subdirectory(tests)