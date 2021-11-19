add_executable(nuspell-exe nuspell.cxx)
set_target_properties(nuspell-exe PROPERTIES RUNTIME_OUTPUT_NAME nuspell)
target_compile_definitions(nuspell-exe PRIVATE
    PROJECT_VERSION=\"${PROJECT_VERSION}\")
target_link_libraries(nuspell-exe Nuspell::nuspell)
if (BUILD_SHARED_LIBS AND WIN32)
    # This should be PRE_LINK (or PRE_BUILD), so Vcpkg's POST_BUILD
    # step (see VCPKG_APPLOCAL_DEPS) that copies dll can pick up nuspell.dll
    # inside the folder ../tools and copy its transitive dependencies.
    add_custom_command(TARGET nuspell-exe PRE_LINK
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:nuspell> $<TARGET_FILE_DIR:nuspell-exe>)
endif()
if (NOT subproject)
    install(TARGETS nuspell-exe DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
