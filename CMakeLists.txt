cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)

project(mordor2)

option(BUILD_MORDOR2_EXAMPLE "build mordor2 examples" OFF)

set(MORDOR2_LIB_SRCS src/config.cxx src/log.cxx src/timestamp.cxx)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

add_library(${PROJECT_NAME} STATIC ${MORDOR2_LIB_SRCS})
target_include_directories(${PROJECT_NAME} PUBLIC include)

if(BUILD_MORDOR2_EXAMPLE)
    add_executable(example examples/example.cxx)
    target_link_libraries(example ${PROJECT_NAME})
endif()
