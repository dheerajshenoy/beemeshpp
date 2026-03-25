cmake_minimum_required(VERSION 3.16)
project(beemesh VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(asio INTERFACE)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/BeeMesh.cpp
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
