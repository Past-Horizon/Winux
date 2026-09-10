set(WINUX_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.12)
  file(GLOB_RECURSE WINUX_SOURCES CONFIGURE_DEPENDS
    "${WINUX_SOURCE_ROOT}/*.cpp"
    "${WINUX_SOURCE_ROOT}/*.h"
  )
else()
  file(GLOB_RECURSE WINUX_SOURCES
    "${WINUX_SOURCE_ROOT}/*.cpp"
    "${WINUX_SOURCE_ROOT}/*.h"
  )
endif()

list(FILTER WINUX_SOURCES EXCLUDE REGEX "[/\\\\]out[/\\\\]")
list(FILTER WINUX_SOURCES EXCLUDE REGEX "[/\\\\]\.git[/\\\\]")

if (NOT WINUX_SOURCES)
  message(FATAL_ERROR
    "No Winux .cpp or .h sources were found under ${WINUX_SOURCE_ROOT}."
  )
endif()
