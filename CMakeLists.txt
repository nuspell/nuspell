cmake_minimum_required(VERSION 3.18)
project(nuspell
	VERSION 5.1.8
	DESCRIPTION "Nuspell spellchecking library"
	HOMEPAGE_URL https://nuspell.github.io/
	LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CMakeDependentOption)

option(BUILD_SHARED_LIBS "Build as shared library" ON)
option(BUILD_TESTING "Build the testing tree." ON)
option(BUILD_TOOLS "Build the CLI tool." ON)
option(BUILD_DOCS "Build the docs." ON)
cmake_dependent_option(BUILD_MAN "Build man-pages." ON BUILD_DOCS OFF)
cmake_dependent_option(BUILD_API_DOCS "Build API docs." ON BUILD_DOCS OFF)

find_package(ICU 60 REQUIRED COMPONENTS uc)
get_directory_property(subproject PARENT_DIRECTORY)
add_subdirectory(src/nuspell)

if (MSVC AND (BUILD_TOOLS OR BUILD_TESTING))
	find_path(GETOPT_INCLUDE_DIR getopt.h REQUIRED)
	find_library(GETOPT_LIBRARY getopt REQUIRED)
endif()
if (BUILD_TOOLS)
	add_subdirectory(src/tools)
endif()
if (subproject)
	# if added as subproject just build Nuspell
	# no need to test, build docs or install
	return()
endif()
if (BUILD_DOCS)
	add_subdirectory(docs)
endif()

macro(fetch_catch2)
	include(FetchContent)
	FetchContent_Declare(Catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2.git
		GIT_TAG 3f0283de7a9c43200033da996ff9093be3ac84dc #v3.3.2
	)
	set(old_build_shared_libs ${BUILD_SHARED_LIBS})
	set(BUILD_SHARED_LIBS OFF)
	FetchContent_MakeAvailable(Catch2)
	target_compile_features(Catch2 PRIVATE cxx_std_17)
	set(BUILD_SHARED_LIBS ${old_build_shared_libs})
endmacro()

if (BUILD_TESTING)
	enable_testing()
	find_package(Catch2 3.1.1 QUIET)
	if (NOT Catch2_FOUND)
		fetch_catch2()
	endif()
	add_subdirectory(external/hunspell/hunspell)
	add_subdirectory(tests)
endif()


file(RELATIVE_PATH PKGCONFIG_LIBDIR
	${CMAKE_INSTALL_PREFIX} ${CMAKE_INSTALL_FULL_LIBDIR})
file(RELATIVE_PATH PKGCONFIG_INCLUDEDIR
	${CMAKE_INSTALL_PREFIX} ${CMAKE_INSTALL_FULL_INCLUDEDIR})
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
