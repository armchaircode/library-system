cmake_minimum_required(VERSION 3.20)
project(library
    DESCRIPTION "Library Management System"
    LANGUAGES CXX)

set(SRC_DIR "src")
set(EXTERNAL_DIR "external")
add_subdirectory("${EXTERNAL_DIR}/FTXUI")
add_subdirectory("${EXTERNAL_DIR}/SQLiteCpp")

file(GLOB_RECURSE SRCFILES "${SRC_DIR}/*.cpp")
include_directories(
    "${SRC_DIR}"
    "${EXTERNAL_DIR}/FXTUI/include"
    "${EXTERNAL_DIR}/SQlite-cpp/include")

add_executable(${PROJECT_NAME} ${SRCFILES})
if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DWINDOWS_TARGET_H)
endif(WIN32)
target_compile_options(${PROJECT_NAME}
    PUBLIC -std=c++20)
target_link_libraries(${PROJECT_NAME}
    ftxui::component
    ftxui::dom
    SQLiteCpp)

