cmake_minimum_required(VERSION 3.10)

project(PolyMPS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Release/Debug)" FORCE)
endif()

file(GLOB_RECURSE POLYMPS_SOURCES CONFIGURE_DEPENDS
  src/*.cpp
)

add_executable(polymps ${POLYMPS_SOURCES})

target_include_directories(polymps PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/eigen
  ${CMAKE_CURRENT_SOURCE_DIR}/json/single_include/nlohmann
  ${CMAKE_CURRENT_SOURCE_DIR}/libigl/include
)

find_package(OpenMP REQUIRED)

target_link_libraries(polymps PRIVATE OpenMP::OpenMP_CXX stdc++fs)

target_compile_definitions(polymps PRIVATE _GNU_SOURCE)
target_compile_options(polymps PRIVATE -O3 -march=native -ffast-math)

set_target_properties(polymps PROPERTIES
  OUTPUT_NAME main
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
