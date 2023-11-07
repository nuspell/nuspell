add_executable(unit_test unit_test.cxx)
target_link_libraries(unit_test PRIVATE nuspell Catch2::Catch2WithMain)
if (MSVC)
	target_compile_options(unit_test PRIVATE "/utf-8")
	# Consider doing this for all the other targets by setting this flag
	# globally for MSVC. ATM we use unicode string literals only here.
endif()

add_executable(legacy_test legacy_test.cxx)
target_link_libraries(legacy_test PRIVATE nuspell)

add_executable(verify verify.cxx)
target_link_libraries(verify PRIVATE nuspell hunspell)
if (MSVC)
	target_include_directories(verify PRIVATE ${GETOPT_INCLUDE_DIR})
	target_link_libraries(verify PRIVATE ${GETOPT_LIBRARY})
endif()

if (BUILD_SHARED_LIBS AND WIN32)
	add_custom_command(TARGET verify PRE_LINK
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:nuspell>
	$<TARGET_FILE:hunspell> $<TARGET_FILE_DIR:unit_test>)
endif()

add_test(NAME unit_test COMMAND unit_test)

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
