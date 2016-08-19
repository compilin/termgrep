cmake_minimum_required(VERSION 3.5)
project(termgrep)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")

include_directories(./include)
link_directories("./lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "./lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "./lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "./bin")

add_library(termgrep src/termgrep.hpp src/termgrep.cpp include/gvpp.hpp)
add_executable(main src/main.cpp)
target_compile_definitions(termgrep PRIVATE CTYPE=char)
target_compile_definitions(main PRIVATE CTYPE=char)

add_library(wtermgrep src/termgrep.hpp src/termgrep.cpp include/gvpp.hpp)
add_executable(wmain src/main.cpp)
target_compile_definitions(wtermgrep PRIVATE CTYPE=wchar_t)
target_compile_definitions(wmain PRIVATE CTYPE=wchar_t)

target_link_libraries(main termgrep gvpp)
target_link_libraries(wmain wtermgrep gvpp)
