cmake_minimum_required(VERSION 3.9)

project(
	opustags
	VERSION 1.2.0
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FindPkgConfig)
pkg_check_modules(OGG REQUIRED ogg)

configure_file(src/config.h.in config.h @ONLY)
include_directories(BEFORE src "${CMAKE_BINARY_DIR}")

add_library(
	libopustags
	OBJECT
	src/cli.cc
	src/ogg.cc
	src/opus.cc
)

add_executable(
	opustags
	src/opustags.cc
	$<TARGET_OBJECTS:libopustags>
)
target_compile_options(opustags PUBLIC ${OGG_CFLAGS})
target_link_libraries(opustags PUBLIC ${OGG_LIBRARIES})

include(GNUInstallDirs)
install(TARGETS opustags DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES opustags.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")

add_subdirectory(t)
