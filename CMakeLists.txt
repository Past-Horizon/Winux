cmake_minimum_required (VERSION 3.10)

project ("Winux")

include ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/WinuxSources.cmake")

if (WIN32)
  list(FILTER WINUX_SOURCES EXCLUDE REGEX "[/\\\\]Platform[/\\\\]Linux[/\\\\]")
elseif (UNIX)
  list(FILTER WINUX_SOURCES EXCLUDE REGEX "[/\\\\]Platform[/\\\\]Windows[/\\\\]")
endif()

add_library(Winux STATIC ${WINUX_SOURCES})
target_include_directories(Winux PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(Winux PUBLIC cxx_std_20)

