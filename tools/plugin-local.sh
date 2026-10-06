#!/bin/sh
# Helpers for testing hyprblur against the running compositor.
# Loads are temporary: they vanish when Hyprland restarts.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PLUGIN_SO="$ROOT/build/hyprblur.so"
LOCAL_PATH="$ROOT/build/local-plugin-${HYPRLAND_INSTANCE_SIGNATURE:-none}.path"

fail() {
  echo "hyprblur: $1" >&2
  exit "${2:-1}"
}

require_hyprctl() {
  command -v hyprctl >/dev/null 2>&1 || fail "hyprctl not found on PATH (install Hyprland to test locally)"
}

require_built() {
  [ -f "$PLUGIN_SO" ] || fail "plugin not built (run 'mise run build' first): $PLUGIN_SO"
}

running() {
  require_hyprctl
  [ -n "${HYPRLAND_INSTANCE_SIGNATURE:-}" ] || return 1
  hyprctl plugin list >/dev/null 2>&1
}

# The shell cannot reach any compositor. Say whether Hyprland is down or
# just elsewhere so the fix is obvious.
no_instance() {
  if command -v pgrep >/dev/null 2>&1 && pgrep -x Hyprland >/dev/null 2>&1; then
    fail "Hyprland is running but this shell is outside its session (HYPRLAND_INSTANCE_SIGNATURE is unset). Run from a terminal inside Hyprland"
  else
    fail "Hyprland is not running"
  fi
}

loaded() {
  running || return 1
  hyprctl plugin list 2>/dev/null | grep -q "hyprblur"
}

# The loader can retain a DSO after unload and reuse it by path. Keep each
# loaded build immutable at a unique path so a rebuild loads fresh code.
# Only unload copies managed here; never replace a hyprpm-managed plugin.

cmd_status() {
  running || {
    echo "hyprblur: no running Hyprland reachable from this shell"
    exit 1
  }
  if loaded; then
    echo "hyprblur is loaded in the running compositor"
  else
    echo "hyprblur is not loaded"
    exit 1
  fi
}

cmd_load() {
  require_built
  require_hyprctl
  running || no_instance

  if loaded; then
    [ -s "$LOCAL_PATH" ] || fail "hyprblur is already loaded outside this helper; unload it by its own path first"
    cmd_unload
  fi

  snapshot=$(mktemp -d "$ROOT/build/local-plugin-XXXXXX")
  cp "$PLUGIN_SO" "$snapshot/hyprblur.so"
  echo "loading $snapshot/hyprblur.so"
  echo "note: a crashing plugin takes the compositor with it; loads are temporary and vanish on restart"
  hyprctl plugin load "$snapshot/hyprblur.so" || fail "compositor refused the plugin (see 'mise run logs')"
  loaded || fail "load reported success but the plugin is not listed (see 'mise run logs')"
  printf '%s\n' "$snapshot/hyprblur.so" > "$LOCAL_PATH"
  echo "hyprblur is loaded"
}

cmd_unload() {
  require_hyprctl
  running || {
    echo "no Hyprland reachable from this shell, nothing to unload"
    exit 0
  }

  if loaded; then
    [ -s "$LOCAL_PATH" ] || fail "hyprblur was loaded outside this helper; unload it by its own path"
    IFS= read -r path < "$LOCAL_PATH"
    hyprctl plugin unload "$path" || fail "unload failed (see 'mise run logs')"
    loaded && fail "hyprblur is still loaded; refusing to load another copy"
    rm -f "$LOCAL_PATH"
    echo "hyprblur is unloaded"
  else
    echo "hyprblur is not loaded, nothing to unload"
  fi

}

cmd_logs() {
  require_hyprctl
  running || no_instance
  exec hyprctl rollinglog -f
}

case "${1:-}" in
  status) cmd_status ;;
  load) cmd_load ;;
  unload) cmd_unload ;;
  logs) cmd_logs ;;
  *) fail "usage: $(basename "$0") {status|load|unload|logs}" 2 ;;
esac
