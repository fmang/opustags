cmake_minimum_required(VERSION 3.9)

project(
	opustags
	VERSION 1.2.0
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OGG REQUIRED ogg)
add_compile_options(${OGG_CFLAGS})

configure_file(src/config.h.in config.h @ONLY)
include_directories(BEFORE src "${CMAKE_BINARY_DIR}")

add_library(
	libopustags
	OBJECT
	src/cli.cc
	src/ogg.cc
	src/opus.cc
)
target_link_libraries(libopustags PUBLIC ${OGG_LIBRARIES})

add_executable(opustags src/opustags.cc)
target_link_libraries(opustags libopustags)

include(GNUInstallDirs)
install(TARGETS opustags DESTINATION "${CMAKE_INSTALL_BINDIR}")
configure_file(opustags.1 . @ONLY)
install(FILES "${CMAKE_BINARY_DIR}/opustags.1" DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")

add_subdirectory(t)
