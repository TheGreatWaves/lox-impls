cmake_minimum_required(VERSION 3.16)

project(sugoi)

set(CMAKE_CXX_STANDARD 20)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

add_executable(${PROJECT_NAME} "src/sugoi.cpp"
    "src/scanner.cpp"
    "src/value.cpp"
 "src/opcode.cpp"
 "src/compiler.cpp"
 "src/vm.cpp")

#target_include_directories(${PROJECT_NAME} 
#	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)