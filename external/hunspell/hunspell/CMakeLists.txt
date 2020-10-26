add_library(hunspell affentry.cxx affentry.hxx affixmgr.cxx affixmgr.hxx
    atypes.hxx baseaffix.hxx csutil.cxx csutil.hxx
    filemgr.cxx filemgr.hxx hashmgr.cxx hashmgr.hxx
    htypes.hxx hunspell.cxx hunspell.h hunspell.hxx
    hunzip.cxx hunzip.hxx langnum.hxx hunvisapi.h
    phonet.cxx phonet.hxx replist.cxx replist.hxx
    suggestmgr.cxx suggestmgr.hxx utf_info.hxx w_char.hxx)
set_target_properties(hunspell PROPERTIES
    DEFINE_SYMBOL BUILDING_LIBHUNSPELL
    CXX_VISIBILITY_PRESET hidden)
if (NOT BUILD_SHARED_LIBS)
    target_compile_definitions(hunspell PUBLIC HUNSPELL_STATIC)
endif()
target_include_directories(hunspell INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
if (MSVC)
    # Supress some useless warnings
    target_compile_definitions(hunspell PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(hunspell PRIVATE /wd4251)
endif()
