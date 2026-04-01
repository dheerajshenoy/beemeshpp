cmake_minimum_required(VERSION 3.16)
project(beemesh VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BEEMESH_DEBUG_FLAGS "-ggdb -Wall -Wextra -Wpedantic -Werror")
set(BEEMESH_RELEASE_FLAGS "-O3 -Wall -Wextra -Wpedantic -Werror")

# 6. Asio requires threads
find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
)

FetchContent_MakeAvailable(asio)

add_library(asio_external INTERFACE)

target_include_directories(asio_external INTERFACE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio_external INTERFACE ASIO_STANDALONE)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/Job.cpp
    src/Hive.cpp
    src/Bee.cpp
    src/Connection.cpp
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


target_link_libraries(${PROJECT_NAME}
    PRIVATE
    asio_external
    Threads::Threads
)
