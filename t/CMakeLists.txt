add_executable(opus.t EXCLUDE_FROM_ALL opus.cc)
target_link_libraries(opus.t libopustags)

add_executable(ogg.t EXCLUDE_FROM_ALL ogg.cc)
target_link_libraries(ogg.t libopustags)

add_executable(cli.t EXCLUDE_FROM_ALL cli.cc)
target_link_libraries(cli.t libopustags)

configure_file(gobble.opus . COPYONLY)

add_custom_target(
	check
	COMMAND prove "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}"
	DEPENDS opustags gobble.opus opus.t ogg.t cli.t
)
