add_executable(system.t EXCLUDE_FROM_ALL system.cc)
target_link_libraries(system.t ot)

add_executable(opus.t EXCLUDE_FROM_ALL opus.cc)
target_link_libraries(opus.t ot)

add_executable(ogg.t EXCLUDE_FROM_ALL ogg.cc)
target_link_libraries(ogg.t ot)

add_executable(cli.t EXCLUDE_FROM_ALL cli.cc)
target_link_libraries(cli.t ot)

add_executable(oggdump EXCLUDE_FROM_ALL oggdump.cc)
target_link_libraries(oggdump ot)

configure_file(gobble.opus . COPYONLY)

add_custom_target(
	check
	COMMAND prove "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}"
	DEPENDS opustags gobble.opus system.t opus.t ogg.t cli.t
)
