# dwm

Custom, highly-optimized build of [dwm](https://dwm.suckless.org/) (dynamic window manager) 6.8 with native asynchronous status blocks, status2d pill capsules, vanity gaps, and seamless session preservation.

## Features

- **Embedded Asynchronous Status Bar**: Built directly into the event loop via `poll()` — no external status daemon (`dwmblocks`, etc.) needed. Zero fork for native sysfs battery and time metrics; non-blocking pipes for shell commands.
- **Status2D Pill Badges**: Independent foreground/background pill geometry (`user_bh`, `pill_vpad`, `pill_hpad`) and decoupled cap/value color schemes (`cap_fg`, `cap_bg`, `val_fg`, `val_bg`).
- **Vanity Gaps & Cfacts**: Tiled layout with configurable inner/outer gaps, smart gaps, and per-client fact resizing.
- **Seamless Fullscreen Handling**: Fixes browser reload freezes in fullscreen, acknowledges `ConfigureRequest` events, and adds a borderless screen cover (`togglefakefullscreen` on `Mod+Ctrl+f`) that keeps floating windows hovering on top.
- **State Preservation on Restart**: Preserves active tags, monitor selection, and focused client across in-place restarts (`Mod+Shift+r`).
- **Per-Tag State (`pertag`)**: Independent layouts, mfacts, and bar visibility per workspace tag.

## Requirements

- Xlib, Xinerama, Xft, and Fontconfig development libraries.
- Recommended font: `IosevkaTerm Nerd Font Mono` (or configure your preferred font in `config.h`).

On Debian / Ubuntu:
```sh
sudo apt install build-essential libx11-dev libxinerama-dev libxft-dev libfontconfig1-dev
```

On Arch Linux:
```sh
sudo pacman -S base-devel libx11 libxinerama libxft fontconfig
```

## Installation

```sh
make
sudo make install
```

## Running

Add the following to your `~/.xinitrc` or startup script:

```sh
exec dwm
```

## Keybindings

| Keybinding | Action |
|---|---|
| `Mod + Return` | Open terminal (`alacritty`) |
| `Mod + d` | App launcher (`rofi`) |
| `Mod + p` | Run menu (`dmenu`) |
| `Mod + q` | Close focused window |
| `Mod + j` / `k` | Focus stack down / up |
| `Mod + h` / `l` | Decrease / increase master width (`mfact`) |
| `Mod + Shift + h` / `l` | Increase / decrease client vertical factor (`cfact`) |
| `Mod + Space` | Toggle between layouts |
| `Mod + Shift + Space` | Toggle floating for window |
| `Mod + Shift + f` | Toggle actual fullscreen (`_NET_WM_STATE_FULLSCREEN`) |
| `Mod + Ctrl + f` | Toggle fake fullscreen (borderless cover with floating on top) |
| `Mod + b` | Toggle status bar visibility |
| `Mod + Ctrl + =` / `-` | Increase / decrease vanity gaps |
| `Mod + Ctrl + g` | Toggle vanity gaps on / off |
| `Mod + Shift + r` | Restart dwm in-place (reloads config while preserving windows) |
| `Mod + Shift + q` | Quit dwm session |
