add_executable(unit_test
    condition_test.cxx
    dictionary_test.cxx
    locale_utils_test.cxx
    string_utils_test.cxx
    structures_test.cxx
    catch_main.cxx)
target_link_libraries(unit_test nuspell Catch2::Catch2)

add_executable(legacy_test legacy_test.cxx)
target_link_libraries(legacy_test nuspell)

add_executable(verify verify.cxx)
target_link_libraries(verify nuspell hunspell Boost::locale)

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
allcaps.dic
allcaps2.dic
allcaps_utf.dic
base_utf.dic
checkcompoundrep.dic
forceucase.dic
hu.dic
nepali.dic
1463589.sug
1463589_utf.sug
allcaps.sug
allcaps2.sug
allcaps_utf.sug
base.sug
base_utf.sug
breakdefault.sug
checksharps.sug
checksharpsutf.sug
forceucase.sug
i35725.sug
i54633.sug
i58202.sug
keepcase.sug
nosuggest.sug
oconv.sug
opentaal_keepcase.sug
phone.sug
sug.sug
sugutf.sug
utf8_nonbmp.sug

PROPERTIES WILL_FAIL TRUE)
