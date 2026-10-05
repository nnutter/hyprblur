# Unit tests for the compositor-free core. They build and run on macOS as
# well as Linux; only the plugin itself needs Hyprland headers.
file(
  GLOB TEST_SOURCES
  CONFIGURE_DEPENDS
  "*.cpp"
)

if(NOT TEST_SOURCES)
  message(STATUS "No tests yet, skipping test targets")
  return()
endif()

find_package(Catch2 3 QUIET)

if(NOT Catch2_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(Catch2)
endif()

add_executable(hyprblur_tests ${TEST_SOURCES})
target_link_libraries(
  hyprblur_tests
  PRIVATE hyprblur_core hyprblur_warnings Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(hyprblur_tests)
