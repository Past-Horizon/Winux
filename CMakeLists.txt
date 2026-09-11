cmake_minimum_required (VERSION 3.10)

project ("Winux")

option(WINUX_BUILD_TESTS "Build Winux tests" OFF)

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

