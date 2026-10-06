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
if hl.plugin and hl.plugin.hyprblur and hl.plugin.hyprblur.toggle then
  hl.bind("SUPER + B", hl.plugin.hyprblur.toggle)
end
```

## Lua Config

Configure the plugin after it loads:

```lua
hl.config({
  plugin = {
    hyprblur = {
      enabled             = true,
      blur_by_default     = false,
      blur_class          = "",
      blur_title          = "",
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
Fade and enable settings apply on every frame, so reloading the config takes effect at once.
`blur_by_default`, `blur_class`, and `blur_title` set the initial blur state only when a window is first tracked.


| Setting              | Default | Meaning                                                     |
| -------------------- | ------- | ----------------------------------------------------------- |
| `enabled`            | `true`  | Master switch for the whole plugin.                         |
| `blur_by_default`    | `false` | New windows blur once they lose focus.                      |
| `blur_class`         | `""`    | Initially enable blur for matching app classes.             |
| `blur_title`         | `""`    | Initially enable blur for matching window titles.           |
| `focus_lost_delay_ms`| `300`   | Wait after losing focus before the fade starts.             |
| `fade_rate_percent_per_ms` | `0.1` | Percentage points of blur added per ms; must be positive. |
| `blur_strength`      | `0.90`  | Final blur amount from `0` (clear) to `1` (full).           |
| `suppress_on_hover`  | `true`  | Keep a window clear while the pointer is over it.           |
| `ensure_global_blur` | `true`  | Turn on global blur after each config reload if it is off.                   |


The blur itself comes from your normal `decoration.blur` settings; the strength above only decides how much of it shows through.

## Automatically blur windows

hyprblur supports automatically enabling blue based on a window's app class or title by specifying a (ECMAScript) regular expression in `blur_class` or `blur_title`.
Rules are evaluated only when a window is first tracked, including already-open windows after the plugin loads and its configuration is read.
Pressing the toggle binding disables blur for that window until you toggle it again or close it.

Prefer matching the app class if window titles can change dynamically based on state/content.

### Example: Automatically blur Signal windows

Run `hyprctl clients` to find Signal's `class`.

Add configuration, similar to the following, after the plugin loads to opt Signal windows into blur without enabling it for other apps,

```lua
if hl.plugin and hl.plugin.hyprblur and hl.plugin.hyprblur.blue_class then
  hl.config({
    plugin = {
      hyprblur = {
        blur_class = "^(Signal|signal)$",
      },
    },
  })
end
```

## Development

```
mise run build         # compile the plugin and the tests
mise run test          # compile and run the unit tests
mise run format        # reformat sources in place
mise run format-check  # fail if anything needs reformatting
mise run lint          # static analysis (cppcheck, plus clang-tidy on src/)
mise run on-stop       # everything above, for the mise-hooks extension
```

### Trying it locally

```
mise run install       # build and load a fresh local copy
mise run reload        # rebuild and replace the local copy
mise run status        # show whether it is loaded
mise run logs          # follow the compositor log (Ctrl-C to stop)
mise run uninstall     # unload the local copy
```
