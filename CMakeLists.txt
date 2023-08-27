cmake_minimum_required(VERSION 3.12)
project(nuspell
	VERSION 5.1.3
	DESCRIPTION "Nuspell spellchecking library"
	HOMEPAGE_URL https://nuspell.github.io/
	LANGUAGES CXX)

option(BUILD_SHARED_LIBS "Build as shared library" ON)
option(BUILD_TESTING "Build the testing tree." ON)
option(BUILD_TOOLS "Build the CLI tool." ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
find_package(ICU 60 REQUIRED COMPONENTS uc data)
get_directory_property(subproject PARENT_DIRECTORY)
add_subdirectory(src/nuspell)

if (MSVC AND (BUILD_TOOLS OR BUILD_TESTING))
	find_path(GETOPT_INCLUDE_DIR getopt.h)
	if (NOT GETOPT_INCLUDE_DIR)
		message(FATAL_ERROR "Can not find getopt.h")
	endif()
	find_library(GETOPT_LIBRARY getopt)
	if (NOT GETOPT_LIBRARY)
		message(FATAL_ERROR "Can not find library getopt")
	endif()
endif()
if (BUILD_TOOLS)
	add_subdirectory(src/tools)
endif()
if (subproject)
	# if added as subproject just build Nuspell
	# no need to test, build docs or install
	return()
endif()

macro(fetch_catch2)
	include(FetchContent)
	FetchContent_Declare(Catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2.git
		GIT_TAG 62fd660583d3ae7a7886930b413c3c570e89786c #v2.13.9
	)
        FetchContent_GetProperties(Catch2)
	if(NOT catch2_POPULATED)
		FetchContent_Populate(Catch2)
		add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
		list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
	endif()
endmacro()

if (BUILD_TESTING)
	enable_testing()
	find_package(Catch2 2.3.0 QUIET)
	if (NOT Catch2_FOUND)
		fetch_catch2()
	endif()
	add_subdirectory(external/hunspell/hunspell)
	add_subdirectory(tests)
endif()


configure_file(nuspell.pc.in nuspell.pc @ONLY)
#configure_file(NuspellConfig.cmake NuspellConfig.cmake COPYONLY)
write_basic_package_version_file(NuspellConfigVersion.cmake
	COMPATIBILITY AnyNewerVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/nuspell.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/NuspellConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/NuspellConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nuspell)
install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
