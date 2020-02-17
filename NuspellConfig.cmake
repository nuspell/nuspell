include(CMakeFindDependencyMacro)
find_dependency(ICU COMPONENTS uc data)
find_dependency(Boost 1.62.0)
include("${CMAKE_CURRENT_LIST_DIR}/NuspellTargets.cmake")
