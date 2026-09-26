cmake_minimum_required (VERSION 3.10)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/WinuxVersion.txt" WINUX_VERSION)
string(STRIP "${WINUX_VERSION}" WINUX_VERSION)
if(NOT WINUX_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
  message(FATAL_ERROR "WinuxVersion.txt must contain a semantic version")
endif()

project ("Winux" VERSION "${WINUX_VERSION}")

if(WIN32)
  set(WINUX_PLATFORM "Windows")
elseif(UNIX)
  set(WINUX_PLATFORM "Linux")
else()
  message(FATAL_ERROR "Unsupported Winux platform: ${CMAKE_SYSTEM_NAME}")
endif()

option(WINUX_BUILD_TESTS "Build Winux tests" OFF)

include ("${CMAKE_CURRENT_SOURCE_DIR}/cmake/WinuxSources.cmake")

if (WIN32)
  list(FILTER WINUX_SOURCES EXCLUDE REGEX "[/\\\\]Platform[/\\\\]Linux[/\\\\]")
elseif (UNIX)
  list(FILTER WINUX_SOURCES EXCLUDE REGEX "[/\\\\]Platform[/\\\\]Windows[/\\\\]")
endif()

add_library(Winux STATIC ${WINUX_SOURCES})
set(WINUX_OUTPUT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/WinuxBuild/${WINUX_PLATFORM}")
set_target_properties(Winux PROPERTIES
  OUTPUT_NAME "Winux.${WINUX_PLATFORM}.$<CONFIG>.${WINUX_VERSION}"
  ARCHIVE_OUTPUT_DIRECTORY "${WINUX_OUTPUT_ROOT}/$<CONFIG>"
)
target_include_directories(Winux PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(Winux PUBLIC cxx_std_20)

if (WIN32)
  target_link_libraries(Winux PRIVATE Shell32)
endif()

if (WINUX_BUILD_TESTS)
  include(FetchContent)

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  add_executable(WinuxProcessTests
    tests/ProcessTests.cpp
    tests/FileSystemTests.cpp
    tests/MutexTests.cpp
    tests/SystemTests.cpp
    tests/LocaleTests.cpp
    tests/TerminalTests.cpp
  )

  target_link_libraries(WinuxProcessTests
    PRIVATE
      Winux
      GTest::gtest_main
  )

  target_compile_features(WinuxProcessTests PRIVATE cxx_std_20)

  include(GoogleTest)
  gtest_discover_tests(WinuxProcessTests)
endif()

