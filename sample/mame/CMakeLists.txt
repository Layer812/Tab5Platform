# Platform -> sample dependency is forbidden. App -> sample -> platform only.
set(mame_sample_sources "src/t5_sample_mame.c")
set(t5_mame_have_external_core FALSE)
# Opt-in ONLY after the upstream MAME 2000 libretro source is actually ported
# to ESP32-P4 and its build sources are explicitly reviewed and wired.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/local_core_sources.cmake")
  include("${CMAKE_CURRENT_LIST_DIR}/local_core_sources.cmake")
  # That local (non-distributed) file must append actual files to mame_sample_sources.
  if(NOT T5_MAME_CORE_PORT_READY)
    message(FATAL_ERROR "local_core_sources.cmake must mark a completed core port")
  endif()
  set(t5_mame_have_external_core TRUE)
endif()
idf_component_register(SRCS ${mame_sample_sources}
  INCLUDE_DIRS "include"
  REQUIRES tab5_platform
  PRIV_REQUIRES freertos log)

if(t5_mame_have_external_core)
  target_compile_definitions(${COMPONENT_LIB} PRIVATE T5_SAMPLE_MAME_WITH_LIBRETRO_CORE=1)
endif()
