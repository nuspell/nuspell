cmake_minimum_required(VERSION 3.7)
project(nuspell VERSION 2.2.0)
set(PROJECT_HOMEPAGE_URL "https://nuspell.github.io/")
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)
find_package(ICU REQUIRED COMPONENTS uc data)
find_package(Boost 1.62.0 REQUIRED COMPONENTS locale)

if (NOT subproject)
add_subdirectory(src/hunspell)
endif()

add_subdirectory(src/nuspell)

get_directory_property(subproject PARENT_DIRECTORY)
if (NOT subproject)
if (BUILD_TESTING)
    execute_process(COMMAND git submodule update --init -- external/Catch2
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_subdirectory(external/Catch2)
    add_subdirectory(tests)
endif()

add_subdirectory(docs)

#set(pkgconf_public_libs "")
set(pkgconf_public_requires icu-uc)
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
endif() # if (NOT subproject)
