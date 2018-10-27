cmake_minimum_required(VERSION 3.5)
project(nuspell VERSION 2.2.0)
set(PROJECT_HOMEPAGE_URL "https://nuspell.github.io/")
include(FindPkgConfig)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

pkg_check_modules(ICU REQUIRED icu-uc)
find_package(Boost 1.62.0 REQUIRED COMPONENTS locale)

add_subdirectory(src/hunspell)
add_subdirectory(src/nuspell)

include(CTest)
execute_process(COMMAND git submodule update --init -- external/Catch2
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(external/Catch2)
add_subdirectory(tests)

add_subdirectory(docs)
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
