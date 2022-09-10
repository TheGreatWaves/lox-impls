cmake_minimum_required(VERSION 3.16)

project(sugoi)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME}
    src/sugoi.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)