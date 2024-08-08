if (BUILD_MAN)
	find_program(PANDOC_EXECUTABLE pandoc)

	if (NOT PANDOC_EXECUTABLE)
		message(FATAL_ERROR "Pandoc not found, can not build the "
			"man-pages. Either install Pandoc or disable builing "
			"the man-pages with cmake -DBUILD_MAN=OFF.")
	endif()

	if (BUILD_TOOLS)
		add_custom_command(OUTPUT nuspell.1
			COMMAND ${PANDOC_EXECUTABLE}
			        ${CMAKE_CURRENT_SOURCE_DIR}/nuspell.1.md
				--standalone --output=nuspell.1
				"--metadata=footer:Nuspell ${PROJECT_VERSION}"
			MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/nuspell.1.md
			COMMENT "Building manpage nuspell.1")
		add_custom_target(man_pages ALL DEPENDS nuspell.1)
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/nuspell.1
			DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
	endif()
endif()

if (BUILD_API_DOCS)
	find_package(Doxygen REQUIRED)

	# Project related configuration options
	set(DOXYGEN_BUILTIN_STL_SUPPORT YES)

	# Build related configuration options
	set(DOXYGEN_HIDE_UNDOC_MEMBERS YES)
	set(DOXYGEN_HIDE_UNDOC_CLASSES YES)

	# Configuration options related to the preprocessor
	set(DOXYGEN_MACRO_EXPANSION YES)
	set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
	set(DOXYGEN_INCLUDE_PATH ${PROJECT_BINARY_DIR}/src/nuspell)
	set(DOXYGEN_PREDEFINED
		NUSPELL_BEGIN_INLINE_NAMESPACE= NUSPELL_END_INLINE_NAMESPACE=
		NUSPELL_EXPORT= NUSPELL_DEPRECATED_EXPORT=) # Define to empty

	# Configuration options related to diagram generator tools
	set(DOXYGEN_DOT_IMAGE_FORMAT svg)

	# consider using argument USE_STAMP_FILE from CMake v3.16
	doxygen_add_docs(api_docs nuspell ALL
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src)
	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
		DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()
