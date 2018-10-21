add_library(nuspell
aff_data.cxx     aff_data.hxx
dictionary.cxx   dictionary.hxx
finder.cxx       finder.hxx
locale_utils.cxx locale_utils.hxx
                 string_utils.hxx
structures.cxx   structures.hxx)

target_compile_features(nuspell PUBLIC
    cxx_aggregate_default_initializers
    cxx_auto_type
    cxx_constexpr
    cxx_default_function_template_args
    cxx_defaulted_functions
    cxx_defaulted_move_initializers
    cxx_delegating_constructors
    cxx_deleted_functions
    cxx_extern_templates
    cxx_final
    cxx_generalized_initializers
    cxx_generic_lambdas
    cxx_inheriting_constructors
    cxx_inline_namespaces
    cxx_lambdas
    cxx_lambda_init_captures
    cxx_noexcept
    cxx_nullptr
    cxx_override
    cxx_range_for
    cxx_return_type_deduction
    cxx_right_angle_brackets
    cxx_rvalue_references
    cxx_static_assert
    cxx_strong_enums
    cxx_thread_local
    cxx_trailing_return_types
    cxx_unicode_literals
    cxx_uniform_initialization
    cxx_user_literals
    cxx_variable_templates
    cxx_variadic_macros
    cxx_variadic_templates)

target_include_directories(nuspell
    PRIVATE ${ICU_INCLUDE_DIRS}
    INTERFACE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(nuspell
    PUBLIC Boost::boost
    PRIVATE Boost::locale
    PRIVATE ${ICU_LIBRARIES})

install(TARGETS nuspell DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES
aff_data.hxx
dictionary.hxx
finder.hxx
locale_utils.hxx
string_utils.hxx
structures.hxx
DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nuspell)

add_executable(nuspell-bin main.cxx)
set_target_properties(nuspell-bin PROPERTIES
    OUTPUT_NAME nuspell)
target_link_libraries(nuspell-bin nuspell)

install(TARGETS nuspell-bin DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(verify verify.cxx)
target_link_libraries(verify nuspell hunspell)
