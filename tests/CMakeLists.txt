add_executable(unit_test
    aff_data_test.cxx
    dictionary_test.cxx
    structures_test.cxx
    utils_test.cxx
    catch_main.cxx)
target_link_libraries(unit_test nuspell Catch2::Catch2)
if (MSVC)
    target_compile_options(unit_test PRIVATE "/utf-8")
    # Consider doing this for all the other targets by setting this flag
    # globally for MSVC. ATM we use unicode string literals only in the tests.
endif()

add_executable(legacy_test legacy_test.cxx)
target_link_libraries(legacy_test nuspell)
add_test(NAME main_whitespace_segmentation
    COMMAND ../src/nuspell/nuspell -s -d ${CMAKE_CURRENT_SOURCE_DIR}/v1cmdline/base_utf ${CMAKE_CURRENT_SOURCE_DIR}/base_utf_segmentations.txt)
add_test(NAME main_unicode_segmentation
    COMMAND ../src/nuspell/nuspell -d ${CMAKE_CURRENT_SOURCE_DIR}/v1cmdline/base_utf ${CMAKE_CURRENT_SOURCE_DIR}/base_utf_segmentations.txt)
add_test(NAME verify_base_utf_wrong
    COMMAND verify -d ${CMAKE_CURRENT_SOURCE_DIR}/v1cmdline/base_utf ${CMAKE_CURRENT_SOURCE_DIR}/v1cmdline/base_utf.good ${CMAKE_CURRENT_SOURCE_DIR}/v1cmdline/base_utf.wrong)
add_test(NAME verify_base_utf_good_and_corrections
    COMMAND verify -f -d ${CMAKE_CURRENT_SOURCE_DIR}/v1cmdline/base_utf ${CMAKE_CURRENT_SOURCE_DIR}/v1cmdline/base_utf.good -c ${CMAKE_CURRENT_SOURCE_DIR}/base_utf_corrections.tsv)

add_executable(verify verify.cxx)
target_link_libraries(verify nuspell hunspell Boost::locale)

if (BUILD_SHARED_LIBS AND WIN32)
    add_custom_command(TARGET unit_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:nuspell> $<TARGET_FILE_DIR:unit_test>)
endif()

include(Catch)
catch_discover_tests(unit_test)

file(GLOB v1tests
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/v1cmdline
    "v1cmdline/*.dic"
    "v1cmdline/*.sug")
foreach(t ${v1tests})
    add_test(
        NAME ${t}
        COMMAND legacy_test ${CMAKE_CURRENT_SOURCE_DIR}/v1cmdline/${t})
endforeach()

set_tests_properties(
base_utf.dic
nepali.dic
checksharps.sug
checksharpsutf.sug
nosuggest.sug
phone.sug
utf8_nonbmp.sug

PROPERTIES WILL_FAIL TRUE)
