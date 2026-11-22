add_executable(system.t EXCLUDE_FROM_ALL system.cc)
target_link_libraries(system.t ot)

add_executable(opus.t EXCLUDE_FROM_ALL opus.cc)
target_link_libraries(opus.t ot)

add_executable(ogg.t EXCLUDE_FROM_ALL ogg.cc)
target_link_libraries(ogg.t ot)

add_executable(cli.t EXCLUDE_FROM_ALL cli.cc)
target_link_libraries(cli.t ot)

add_executable(base64.t EXCLUDE_FROM_ALL base64.cc)
target_link_libraries(base64.t ot)

add_executable(oggdump EXCLUDE_FROM_ALL oggdump.cc)
target_link_libraries(oggdump ot)

configure_file(gobble.opus . COPYONLY)
configure_file(pixel.png . COPYONLY)

if(WIN32)
	# prove is a Perl script, which Windows cannot run by itself.
	find_package(Perl REQUIRED)
	execute_process(
		COMMAND "${PERL_EXECUTABLE}" -MConfig -e "print \$Config{installscript}"
		OUTPUT_VARIABLE PERL_SCRIPT_DIR
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	file(TO_CMAKE_PATH "${PERL_SCRIPT_DIR}" PERL_SCRIPT_DIR)
	# prove only finds *.t files in directories, and the unit tests are .exe files here.
	add_custom_target(
		check
		COMMAND "${PERL_EXECUTABLE}" "${PERL_SCRIPT_DIR}/prove"
		        $<TARGET_FILE:system.t> $<TARGET_FILE:opus.t> $<TARGET_FILE:ogg.t>
		        $<TARGET_FILE:cli.t> $<TARGET_FILE:base64.t>
		        "${CMAKE_CURRENT_SOURCE_DIR}/opustags.t"
		DEPENDS opustags gobble.opus system.t opus.t ogg.t cli.t base64.t
	)
else()
add_custom_target(
	check
	COMMAND prove "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}"
	DEPENDS opustags gobble.opus system.t opus.t ogg.t cli.t base64.t
)
endif()
