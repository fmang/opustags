add_executable(unit.t EXCLUDE_FROM_ALL unit.cc)
target_link_libraries(unit.t libopustags)

add_custom_target(
	check
	COMMAND prove "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}"
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	DEPENDS opustags unit.t
)
