include(CMakeFindDependencyMacro)
find_dependency(ICU COMPONENTS uc)
include("${CMAKE_CURRENT_LIST_DIR}/NuspellTargets.cmake")
