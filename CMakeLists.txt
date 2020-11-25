cmake_minimum_required(VERSION 3.9)

project(
	opustags
	VERSION 1.5.1
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# opustags is mainly developed with glibc, which introduces a few
# incompatibilites with BSDs, like getline not being defined by default.
# _GNU_SOURCE should trigger BSD’s libc GNU compatibility mode to fix that.
add_definitions(-D_GNU_SOURCE)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OGG REQUIRED ogg)
add_compile_options(${OGG_CFLAGS})
link_directories(${OGG_LIBRARY_DIRS})

include(FindIconv)

# We need endian.h on Linux, and sys/endian.h on BSD.
include(CheckIncludeFileCXX)
check_include_file_cxx(endian.h HAVE_ENDIAN_H)
check_include_file_cxx(sys/endian.h HAVE_SYS_ENDIAN_H)

include(CheckStructHasMember)
check_struct_has_member("struct stat" st_mtim sys/stat.h HAVE_STAT_ST_MTIM LANGUAGE CXX)
check_struct_has_member("struct stat" st_mtimespec sys/stat.h HAVE_STAT_ST_MTIMESPEC LANGUAGE CXX)

configure_file(src/config.h.in config.h @ONLY)
include_directories(BEFORE src "${CMAKE_BINARY_DIR}" ${OGG_INCLUDE_DIRS} ${Iconv_INCLUDE_DIRS})

add_library(
	ot
	STATIC
	src/cli.cc
	src/ogg.cc
	src/opus.cc
	src/system.cc
)
target_link_libraries(ot PUBLIC ${OGG_LIBRARIES} ${Iconv_LIBRARIES})

add_executable(opustags src/opustags.cc)
target_link_libraries(opustags ot)

include(GNUInstallDirs)
install(TARGETS opustags DESTINATION "${CMAKE_INSTALL_BINDIR}")
configure_file(opustags.1 . @ONLY)
install(FILES "${CMAKE_BINARY_DIR}/opustags.1" DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")

add_subdirectory(t)
