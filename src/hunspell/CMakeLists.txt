add_library(hunspell affentry.cxx affentry.hxx affixmgr.cxx affixmgr.hxx
    atypes.hxx baseaffix.hxx csutil.cxx csutil.hxx
    filemgr.cxx filemgr.hxx hashmgr.cxx hashmgr.hxx
    htypes.hxx hunspell.cxx hunspell.h hunspell.hxx
    hunzip.cxx hunzip.hxx langnum.hxx hunvisapi.h
    phonet.cxx phonet.hxx replist.cxx replist.hxx
    suggestmgr.cxx suggestmgr.hxx utf_info.hxx w_char.hxx)
target_compile_definitions(hunspell PUBLIC HUNSPELL_STATIC)
target_include_directories(hunspell INTERFACE ${PROJECT_SOURCE_DIR}/src)
