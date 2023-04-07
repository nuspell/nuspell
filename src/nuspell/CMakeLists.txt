add_library(nuspell
                 defines.hxx
aff_data.cxx     aff_data.hxx
checker.cxx      checker.hxx
suggester.cxx    suggester.hxx
dictionary.cxx   dictionary.hxx
finder.cxx       finder.hxx
                 unicode.hxx
utils.cxx        utils.hxx
                 structures.hxx)

add_library(Nuspell::nuspell ALIAS nuspell)

include(GenerateExportHeader)
generate_export_header(nuspell)

set(nuspell_headers aff_data.hxx checker.hxx suggester.hxx dictionary.hxx
	finder.hxx structures.hxx unicode.hxx defines.hxx
	${CMAKE_CURRENT_BINARY_DIR}/nuspell_export.h)
set_target_properties(nuspell PROPERTIES
	PUBLIC_HEADER "${nuspell_headers}"
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
	CXX_VISIBILITY_PRESET hidden)

target_compile_features(nuspell PUBLIC cxx_std_17)

target_include_directories(nuspell
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
	          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(nuspell PUBLIC ICU::uc ICU::data)

if (subproject)
	return()
endif()

install(TARGETS nuspell
	EXPORT NuspellTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nuspell)
install(EXPORT NuspellTargets
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nuspell
	NAMESPACE Nuspell::)
