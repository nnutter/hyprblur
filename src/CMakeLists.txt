# The timing logic is dependency-free so it can be unit tested without a
# running compositor. The plugin itself only builds where Hyprland is.
file(
  GLOB CORE_HEADERS
  CONFIGURE_DEPENDS
  "*.hpp"
)

add_library(hyprblur_core INTERFACE ${CORE_HEADERS})
target_include_directories(
  hyprblur_core INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(hyprblur_core INTERFACE hyprblur_warnings)

file(
  GLOB PLUGIN_SOURCES
  CONFIGURE_DEPENDS
  "plugin/*.cpp"
)

if(HYPRLAND_FOUND AND PLUGIN_SOURCES)
  pkg_check_modules(
    PLUGIN_DEPS
    REQUIRED
    IMPORTED_TARGET
    libdrm
    libinput
    libudev
    pixman-1
    wayland-server
    xkbcommon
  )

  add_library(hyprblur SHARED ${PLUGIN_SOURCES})
  target_link_libraries(
    hyprblur
    PRIVATE hyprblur_core hyprblur_warnings rt PkgConfig::HYPRLAND
            PkgConfig::PLUGIN_DEPS
  )
  set_target_properties(
    hyprblur PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
  install(TARGETS hyprblur)
elseif(HYPRLAND_FOUND)
  message(STATUS "No plugin sources yet, skipping the plugin target")
endif()
