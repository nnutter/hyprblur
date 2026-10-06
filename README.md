# hyprblur

Blur a window a moment after it loses focus.
Focused windows and the window under the pointer stay clear; blurred windows fade
into the background until you come back to it.

## Install

You need Hyprland headers matching your running Hyprland, plus `hyprpm`:

```
hyprpm add https://github.com/nnutter/hyprblur
hyprpm enable hyprblur
```

## Setup

Add this key binding to your Lua configuration after the plugin loads to toggle blurring for the focused window:

```lua
hl.bind("SUPER + B", hl.plugin.hyprblur.toggle)
```

## Lua Config

Configure the plugin after it loads:

```lua
hl.config({
  plugin = {
    hyprblur = {
      enabled             = true,
      blur_by_default     = false,
      focus_lost_delay_ms = 300,
      fade_rate_percent_per_ms = 0.1,
      blur_strength       = 0.90,
      suppress_on_hover   = true,
      ensure_global_blur  = true,
    },
  },
})
```

All settings live under `plugin.hyprblur` in your Lua configuration.
They apply on every frame, so reloading the config takes effect at once.


| Setting              | Default | Meaning                                                     |
| -------------------- | ------- | ----------------------------------------------------------- |
| `enabled`            | `true`  | Master switch for the whole plugin.                         |
| `blur_by_default`    | `false` | New windows blur once they lose focus.                      |
| `focus_lost_delay_ms`| `300`   | Wait after losing focus before the fade starts.             |
| `fade_rate_percent_per_ms` | `0.1` | Percentage points of blur added per ms; must be positive. |
| `blur_strength`      | `0.90`  | Final blur amount from `0` (clear) to `1` (full).           |
| `suppress_on_hover`  | `true`  | Keep a window clear while the pointer is over it.           |
| `ensure_global_blur` | `true`  | Turn on global blur after each config reload if it is off.                   |


The blur itself comes from your normal `decoration.blur` settings; the strength above only decides how much of it shows through.

## Development

```
mise run build         # compile the plugin and the tests
mise run test          # compile and run the unit tests
mise run format        # reformat sources in place
mise run format-check  # fail if anything needs reformatting
mise run lint          # static analysis (cppcheck, plus clang-tidy on src/)
mise run on-stop       # everything above, for the mise-hooks extension
```

## Trying it locally

```
mise run install       # build and load a fresh local copy
mise run reload        # rebuild and replace the local copy
mise run status        # show whether it is loaded
mise run logs          # follow the compositor log (Ctrl-C to stop)
mise run uninstall     # unload the local copy
```
