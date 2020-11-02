cmake_minimum_required(VERSION 3.8)
project(nuspell VERSION 4.0.1)
set(PROJECT_HOMEPAGE_URL "https://nuspell.github.io/")

option(BUILD_SHARED_LIBS "Buils as shared library" ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(ICU REQUIRED COMPONENTS uc data)
find_package(Boost 1.48.0 REQUIRED COMPONENTS locale)

get_directory_property(subproject PARENT_DIRECTORY)

add_subdirectory(src/nuspell)

if (subproject)
    # if added as subproject just build Nuspell
    # no need to test, build docs or install
    return()
endif()

add_subdirectory(docs)

function(find_catch2_from_source)
    set(Catch2_FOUND Catch2-NOTFOUND PARENT_SCOPE)
    set(catch_cmake_lists ${PROJECT_SOURCE_DIR}/external/Catch2/CMakeLists.txt)
    if (NOT EXISTS ${catch_cmake_lists})
        find_package(Git)
        if (NOT Git_FOUND)
            return()
        endif()
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init -- Catch2
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/external
            RESULT_VARIABLE git_submodule_error
            ERROR_QUIET)
        if (git_submodule_error OR NOT EXISTS ${catch_cmake_lists})
            return()
        endif()
    endif()
    add_subdirectory(external/Catch2)
    list(APPEND CMAKE_MODULE_PATH
         ${PROJECT_SOURCE_DIR}/external/Catch2/contrib)
    set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
    set(Catch2_FOUND 1 PARENT_SCOPE)
endfunction()

option(BUILD_TESTING "Build the testing tree." ON)
if (BUILD_TESTING)
    enable_testing()
    find_package(Catch2 2.3.0 QUIET)
    if (NOT Catch2_FOUND)
        find_catch2_from_source()
    endif()
    if (Catch2_FOUND)
        add_subdirectory(external/hunspell/hunspell)
        add_subdirectory(tests)
    else()
        message(WARNING "Can not find Catch2, tests will be disabled")
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
