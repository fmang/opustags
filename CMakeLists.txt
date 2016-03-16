cmake_minimum_required (VERSION 2.8.8)
project (opustags)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules/")
set(CMAKE_SOURCE_DIR "${CMAKE_BINARY_DIR}/../")

# ------------
# Dependencies
# ------------
find_package(Ogg REQUIRED)
include_directories(${Ogg_INCLUDE_DIR})
link_directories(${Ogg_LIBRARY_DIRS})

# --------------------
# Global build options
# --------------------
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14") # for MinGW-w64
endif()

# -------
# Version
# -------
execute_process(COMMAND git describe --tags --abbrev=0 OUTPUT_VARIABLE VERSION_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git describe --always --dirty --long --tags OUTPUT_VARIABLE VERSION_LONG OUTPUT_STRIP_TRAILING_WHITESPACE)
if("${VERSION_SHORT}" STREQUAL "")
    set(VERSION_SHORT "0.0")
    set(VERSION_LONG "?")
endif()
configure_file("${CMAKE_SOURCE_DIR}/src/version.h.in" "${CMAKE_SOURCE_DIR}/src/version.h" @ONLY)

# ------------
# Source files
# ------------
file(GLOB_RECURSE common_sources "${CMAKE_SOURCE_DIR}/src/*.cc")
file(GLOB_RECURSE common_headers "${CMAKE_SOURCE_DIR}/src/*.h")
file(GLOB_RECURSE test_sources "${CMAKE_SOURCE_DIR}/tests/*.cc")
file(GLOB_RECURSE test_headers "${CMAKE_SOURCE_DIR}/tests/*.h")
list(REMOVE_ITEM common_sources "${CMAKE_SOURCE_DIR}/src/main.cc")
list(REMOVE_ITEM test_sources "${CMAKE_SOURCE_DIR}/tests/main.cc")

# -------------------
# 3rd party libraries
# -------------------
# Catch
set(CATCH_PATH "${CMAKE_SOURCE_DIR}/tests/catch.h")
if (NOT EXISTS "${CATCH_PATH}")
    message("Downloading Catch...")
    file(DOWNLOAD "http://raw.githubusercontent.com/philsquared/Catch/master/single_include/catch.hpp" "${CATCH_PATH}")
endif()

# -------------------
# Linking definitions
# -------------------
add_library(common OBJECT ${common_sources} ${common_headers})
add_executable(opustags "${CMAKE_SOURCE_DIR}/src/main.cc" $<TARGET_OBJECTS:common>)
add_executable(run_tests "${CMAKE_SOURCE_DIR}/tests/main.cc" $<TARGET_OBJECTS:common> ${test_sources} ${test_headers})
target_link_libraries(opustags ${OGG_LIBRARY})
target_link_libraries(run_tests ${OGG_LIBRARY})
target_include_directories(common BEFORE PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_include_directories(opustags BEFORE PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_include_directories(run_tests BEFORE PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_include_directories(run_tests BEFORE PUBLIC "${CMAKE_SOURCE_DIR}/tests")
