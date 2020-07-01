add_library(nuspell
aff_data.cxx     aff_data.hxx
dictionary.cxx   dictionary.hxx
finder.cxx       finder.hxx
utils.cxx        utils.hxx
                 structures.hxx)

add_library(Nuspell::nuspell ALIAS nuspell)

set(nuspell_headers aff_data.hxx dictionary.hxx finder.hxx structures.hxx)
set_target_properties(nuspell PROPERTIES
    PUBLIC_HEADER "${nuspell_headers}"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_compile_features(nuspell PUBLIC cxx_std_17)

target_include_directories(nuspell
    INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>)

target_link_libraries(nuspell
    PUBLIC Boost::boost ICU::uc ICU::data)

add_executable(nuspell-bin main.cxx)
set_target_properties(nuspell-bin PROPERTIES
    OUTPUT_NAME nuspell)
target_compile_definitions(nuspell-bin PRIVATE
    PROJECT_VERSION=\"${PROJECT_VERSION}\")
target_link_libraries(nuspell-bin nuspell Boost::locale)

if (NOT subproject)
    install(TARGETS nuspell
        EXPORT NuspellTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nuspell)
    install(EXPORT NuspellTargets
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nuspell
        NAMESPACE Nuspell::)
    install(TARGETS nuspell-bin DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
