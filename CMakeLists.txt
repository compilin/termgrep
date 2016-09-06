cmake_minimum_required(VERSION 3.5)
include(ExternalProject)
project(termgrep VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")

find_package(Boost
1.49.0
REQUIRED
COMPONENTS program_options
)

add_subdirectory(./deps/gvpp)
set_target_properties(gvpp_test PROPERTIES EXCLUDE_FROM_ALL true)

include_directories(${gvpp_SOURCE_DIR}/src ./deps/json/src)
#link_directories("./lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "./lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "./lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "./bin")

add_library(termgrep src/termgrep.hpp src/termgrep.cpp)

add_executable(termgrep_main src/main.cpp)
target_compile_definitions(termgrep_main PRIVATE DEFAULT_CTYPE=char)

add_executable(wtermgrep_main src/main.cpp)
target_compile_definitions(wtermgrep_main PRIVATE DEFAULT_CTYPE=wchar_t)

target_link_libraries(termgrep_main termgrep gvpp ${Boost_LIBRARIES})
target_link_libraries(wtermgrep_main termgrep gvpp ${Boost_LIBRARIES})
