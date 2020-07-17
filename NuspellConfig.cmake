include(CMakeFindDependencyMacro)
find_dependency(ICU COMPONENTS uc data)
include("${CMAKE_CURRENT_LIST_DIR}/NuspellTargets.cmake")
