cmake_minimum_required(VERSION 3.16)
project(beemesh VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
)

FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui.git
  GIT_TAG        v5.0.0
  GIT_SHALLOW    TRUE
)

set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS    OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(asio ftxui)

add_library(asio_external INTERFACE)

target_include_directories(asio_external INTERFACE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio_external INTERFACE ASIO_STANDALONE)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/Job.cpp
    src/Hive.cpp
    src/Bee.cpp
    src/Connection.cpp
    src/Launcher.cpp
    src/Monitor.cpp
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4 /WX /utf-8 /permissive-
        $<$<CONFIG:Debug>:/Zi>
        $<$<CONFIG:Release>:/O2>
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        BEEMESH_VERSION="${PROJECT_VERSION}"
        _WIN32_WINNT=0x0601
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE dxgi)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        $<$<CONFIG:Debug>:-ggdb>
        $<$<CONFIG:Release>:-O3>
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        BEEMESH_VERSION="${PROJECT_VERSION}"
    )
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    asio_external
    Threads::Threads
    ftxui::screen
    ftxui::dom
    ftxui::component
)
