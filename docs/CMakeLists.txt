find_program(PANDOC_EXECUTABLE pandoc)

if (NOT PANDOC_EXECUTABLE)
	message(FATAL_ERROR "Pandoc not found, can not build man-pages. "
		"Either install Pandoc or disable builing the docs with "
		"cmake -DBUILD_DOCS=OFF")
endif()

if (BUILD_TOOLS)
	add_custom_command(OUTPUT nuspell.1
		COMMAND
		${PANDOC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/nuspell.1.md
		--from=markdown --to=man --standalone --output=nuspell.1
		"--metadata=footer:Nuspell ${PROJECT_VERSION}"
		MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/nuspell.1.md
		COMMENT "Building manpage nuspell.1")
	add_custom_target(nuspell-man-pages ALL DEPENDS nuspell.1)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/nuspell.1
		DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif()
