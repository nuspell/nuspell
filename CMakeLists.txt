cmake_minimum_required(VERSION 3.7)
project(nuspell VERSION 2.3.0)
set(PROJECT_HOMEPAGE_URL "https://nuspell.github.io/")

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

find_package(ICU REQUIRED COMPONENTS uc data)
find_package(Boost 1.62.0 REQUIRED COMPONENTS locale)

get_directory_property(subproject PARENT_DIRECTORY)

add_subdirectory(src/nuspell)

if (subproject)
    # if added as subproject just build Nuspell
    # no need to test, build docs or install
    return()
endif()

add_subdirectory(docs)

if (BUILD_TESTING)
    find_package(Catch2 2.3.0 QUIET)
    if(NOT Catch2_FOUND)
        find_package(Git)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init -- Catch2
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external
            RESULT_VARIABLE git_submodule_error
            ERROR_QUIET)
        if (NOT git_submodule_error)
            add_subdirectory(external/Catch2)
            list(APPEND CMAKE_MODULE_PATH
                "${PROJECT_SOURCE_DIR}/external/Catch2/contrib")
        endif()
    endif()
    if (NOT git_submodule_error)
        add_subdirectory(src/hunspell)
        add_subdirectory(tests)
    else()
        message(WARNING "Can not pull Catch2 submodule, tests will be disabled")
    endif()
endif()


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
