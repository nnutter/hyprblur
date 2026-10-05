cmake_minimum_required(VERSION 3.27)

project(
  hyprblur
  VERSION 0.1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(HYPRBLUR_BUILD_TESTS "Build the unit tests" ON)
option(HYPRBLUR_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

add_library(hyprblur_warnings INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(hyprblur_warnings INTERFACE -Wall -Wextra -Wpedantic)
  if(HYPRBLUR_WARNINGS_AS_ERRORS)
    target_compile_options(hyprblur_warnings INTERFACE -Werror)
  endif()
endif()

# Hyprland ships its pkg-config files outside the default search path on some systems, so look in the usual distro
# locations as well.
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  foreach(
    candidate
    /usr/share/pkgconfig
    /usr/lib/pkgconfig
    /usr/lib64/pkgconfig
    /lib/pkgconfig
    /lib64/pkgconfig
  )
    if(IS_DIRECTORY "${candidate}")
      set(ENV{PKG_CONFIG_PATH} "${candidate}:$ENV{PKG_CONFIG_PATH}")
    endif()
  endforeach()
  pkg_check_modules(HYPRLAND IMPORTED_TARGET hyprland)
endif()

if(HYPRLAND_FOUND)
  message(STATUS "Hyprland headers found, the plugin target is enabled")
else()
  message(STATUS "Hyprland headers not found, skipping the plugin target (core and tests still build)")
endif()

include(CTest)

add_subdirectory(src)

if(HYPRBLUR_BUILD_TESTS)
  add_subdirectory(tests)
endif()
