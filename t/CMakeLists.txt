add_executable(opus.t EXCLUDE_FROM_ALL opus.cc)
target_link_libraries(opus.t libopustags)

add_custom_target(
	check
	COMMAND prove "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}"
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	DEPENDS opustags opus.t
)
