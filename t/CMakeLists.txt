add_executable(
	unit.t
	EXCLUDE_FROM_ALL
	unit.cc
	$<TARGET_OBJECTS:libopustags>
)
target_compile_options(unit.t PUBLIC ${OGG_CFLAGS})
target_link_libraries(unit.t PUBLIC ${OGG_LIBRARIES})

add_custom_target(
	check
	COMMAND prove "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}"
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	DEPENDS opustags unit.t
)
