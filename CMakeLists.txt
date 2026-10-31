cmake_minimum_required(VERSION 3.16)
# Host platform is an independent reusable component. MAME is a SAMPLE,
# never a dependency of the host platform.
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/sample/mame")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(tab5_platform_reference)
