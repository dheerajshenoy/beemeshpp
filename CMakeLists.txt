cmake_minimum_required(VERSION 3.16)
project(beemesh VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BEEMESH_DEBUG_FLAGS "-ggdb -Wall -Wextra -Wpedantic -Werror")
set(BEEMESH_RELEASE_FLAGS "-O3 -Wall -Wextra -Wpedantic -Werror")


add_library(asio INTERFACE)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/BeeMesh.cpp
    src/Job.cpp
    src/Hive.cpp
    src/Bee.cpp
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
    $<$<CONFIG:Debug>:${BEEMESH_DEBUG_FLAGS}>
    $<$<CONFIG:Release>:${BEEMESH_RELEASE_FLAGS}>
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    BEEMESH_VERSION="${PROJECT_VERSION}"
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_include_directories(asio
    INTERFACE
    ${PROJECT_SOURCE_DIR}/external/asio/asio/include
)

target_compile_definitions(asio
    INTERFACE
    ASIO_STANDALONE
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    asio
    pthread
)
