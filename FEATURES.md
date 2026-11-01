# Features & Architecture

A quick tour of what's built into this custom `dwm` build and why it's structured this way.

---

## TL;DR

- **Built-in Status Bar Engine:** No `dwmblocks`, `slstatus`, or external daemons needed. Battery and time run natively in C with zero forks; custom scripts run asynchronously via non-blocking pipes inside dwm's own event loop.
- **Status2D Pill Capsules:** Status widgets render as modern, rounded pill capsules with separate icon and value surfaces.
- **In-Place Seamless Restart (`Mod + Shift + r`):** Recompile and reload dwm in milliseconds without losing open windows, workspace tags, or focus.
- **Vanity Gaps:** Fully adjustable inner and outer window gaps on the fly, with smart gaps for single-window focus.
- **Vertical Stack Sizing (`cfacts`):** Resize individual windows up and down within the tiled stack, not just the master window width.
- **Dual Fullscreen Modes:** Choose between real fullscreen and fake fullscreen (borderless full-area coverage while letting floating tools hover on top).
- **Compositor Geometry Bridge:** Exports exact bar bounds, Status2D pill coordinates, and off-screen window markers via X11 atoms for perfect rounded corners and shadow clipping without 1px bleed.
- **Infallible Session Sanitization:** Tagmasks and monitor indices are validated through proven sanitizers on restart (`Mod + Shift + r`), preventing ghost windows or crashes even across drastic `config.h` changes.
- **ICCCM Size Hints Projection:** Exact 64-bit cross-multiplied integer engine for aspect ratios and character cell increments, eliminating float jitter and infinite resize loops with proven idempotence.
- **Modal Z-Order Tree (`restack`):** Fuel-bounded topological Z-sorter ensures modal dialogs stay visible above their parent windows, preventing UI lockups and cycle hangs.
- **UTF-8 & Status2D Boundary Sanitizer:** Prevents multi-byte glyph slicing and unclosed tag injection in async status scripts, eliminating corrupted font widths and color bleeding.
- **Overflow-Proof Multi-Monitor Voronoi Routing (`recttomon` & `INTERSECT`):** 64-bit coordinate arithmetic eliminates integer overflow on extreme client positions. When a window floats in multi-monitor dead space (0 overlap), Manhattan box distance routes it to the nearest physical monitor rather than defaulting to `selmon`. On monitor disconnect, floating windows are clamped into the remaining workarea, preventing windows lost in the void.
- **Passive Grab & Modifier Bijection (`grabkeys` vs. `keypress`):** Normalizes binding modifier masks with `CLEANMASK` prior to registering passive X11 grabs, mathematically proving exact bijection between X11 grab activation and dwm event dispatch. Eliminates ghost swallowed keys, missed triggers under CapsLock/NumLock, and updates dynamic modifiers on `MappingModifier` events.
- **Floating Bar Island & Pill Hit-Testing Partition (`drawbar` vs. `buttonpress`):** Eliminates coordinate drift between rendered status capsules and click dispatch. Individual Status2D pills are hit-tested to pass 1-based block indices (`arg.i`), while status boundaries are strictly clamped to protect tag and layout symbol click regions.

---

## 1. Built-in Async Status Blocks (No Daemon Needed)

Stock `dwm` usually relies on an external daemon like `dwmblocks` or `slstatus` setting the root window title (`xsetroot -name`). That adds background processes, X11 event noise, and IPC overhead.

This build integrates the status subsystem directly into `dwm.c`:

- **Zero-Fork Native C Metrics:** Things like battery capacity (`/sys/class/power_supply/`) and local time (`strftime`) are read straight from C in microseconds. No shell spawned, no pipes, zero zombie risk.
- **Non-Blocking Async Commands:** Any shell commands (like memory checks or custom scripts) run through non-blocking pipes hooked into dwm's `poll()` loop. Even if a script hangs or lags, your window manager never freezes.
- **Stall Protection:** If a background command exceeds its interval timeout, dwm automatically kills and cleans it up.

You configure your blocks in `config.def.h` / `config.h`:
```c
static Block status_blocks[] = {
    /* icon    command                                func             interval */
    { " ",    "free -h | awk '/^Mem:/ {print $3}'",  NULL,            15 },
    { "󰁹 ",    NULL,                                  status_battery,  30 },
    { "󱑆",     NULL,                                  status_datetime, 30 },
};
```

---

## 2. Status2D Pill Capsules

Instead of a plain text status line, the statusbar supports custom 2D markup with dedicated capsule pill styling:

- **Two-Tone Badges:** Each pill can have a distinct icon surface and data surface (e.g. dark icon box paired with clean data text).
- **Configurable Geometry:** Set exact bar height (`user_bh`), vertical pill inset (`pill_vpad`), and inner pill padding (`pill_hpad`).
- **Color Cache:** Custom hex colors parsed from `^c#hex^` and `^b#hex^` tags are cached in an internal 16-slot LRU table to prevent Xft font/color memory leaks during frequent redraws.

---

## 3. Seamless Session Preservation (`Mod + Shift + r`)

One of the best quality-of-life additions for tweaking your config:

- Pressing `Mod + Shift + r` reloads dwm instantly.
- **Windows Stay Alive:** dwm does not unmanage or close client windows on restart.
- **State Restored:** Tags, monitor assignments, active tags, and the focused window are saved to X11 root properties and reattached when the new binary initializes.
- You can edit `config.h`, run `sudo make install`, hit `Mod + Shift + r`, and continue working uninterrupted.

---

## 4. Vanity Gaps & Smart Gaps

Window spacing is handled via `vanitygaps.c`:

- **Hotkeys on Demand:**
  - `Mod + Ctrl + =` / `-`: Increase or decrease all gaps.
  - `Mod + Ctrl + Shift + =` / `-`: Adjust only inner gaps between windows.
  - `Mod + Ctrl + g`: Toggle all gaps on/off.
  - `Mod + Ctrl + Shift + g`: Reset gaps to default.
- **Smart Gaps:** When `smartgaps` is enabled, outer padding around screen edges disappears when only one window is open on a tag to maximize usable screen real estate.

---

## 5. Client Factors (`cfacts`)

In standard dwm, `mfact` only lets you change the horizontal boundary between the master window and the stack. You can't resize individual windows inside the stack.

With `cfacts`:
- Select any window in the stack.
- Press `Mod + Shift + h` or `Mod + Shift + l` to increase or decrease its relative vertical height.
- Press `Mod + Shift + o` to reset the window back to default proportion.

---

## 6. Fake Fullscreen & True Fullscreen

- **True Fullscreen (`Mod + Shift + f`):** Standard fullscreen. Covers the entire monitor, hides the bar, and restores original geometry and borders when toggled off.
- **Fake Fullscreen (`Mod + Ctrl + f`):** Tricks video players or games into thinking they are in full-screen mode while keeping floating tools, modals, or companion windows accessible on top.

---

## 7. Clean Bar Styling & Aesthetics

- **Active Tag Underline:** A clean underline indicator beneath the current active tag (`ulinepad`, `ulinestroke`, `ulinevoffset`) instead of a heavy filled block.
- **Floating Island Bar Option:** Can be configured to float with screen gaps matching your vanity gap geometry (`floatbar = 1`).
- **Human-Readable Config:** Variables and keybindings in `config.def.h` are organized with clean comments and straightforward naming.

---

## 8. Compositor Geometry & Pill Stencil Bridge

dwm communicates exact geometry boundaries directly to external X11 compositors (Picom, compfy, etc.) via dedicated window properties:

- **`_DWM_BAR_BOUNDS` on `barwin`:** Exports `[x, y, w, h]` of the floating island bar, ensuring compositors can clip rounded corners without 1px gap misalignment.
- **`_DWM_BAR_PILLS` on `barwin`:** Exports `[x, y, w, h]` arrays for every rendered Status2D capsule pill, allowing compositors to apply rounded stencil clipping, shaders, or blur exactly to each pill badge.
- **`_DWM_FAKE_FULLSCREEN` on `c->win`:** Marks fake-fullscreen clients so compositors can selectively disable window shadows or manage Z-depth without full EWMH override.
- **`_DWM_WIN_HIDDEN` on `c->win`:** Explicitly signals when a window is parked off-screen (`WIDTH(c) * -2`), eliminating shadow bleeding or stale blur artifacts across inactive tags.
- **Formally Verified in Lean 4:** All bounding geometries, containment guarantees, and disjointness boundaries are mathematically proven in `DwmTile.lean` using Lean 4's `omega` solver.

---

## 9. Infallible State Sanitization Across Restart

When `config.h` changes between restarts (e.g. changing tag counts or unplugging a secondary monitor) and you press `Mod + Shift + r`:

- **Zero-Tag Protection (`Rule T1`):** `sanitize_tags()` mathematically ensures `c->tags & TAGMASK != 0` for all $2^{32}$ inputs, preventing windows from turning into permanently invisible "ghost" windows parked off-screen.
- **Dangling Monitor Guard (`Rule L2`):** `sanitize_mon()` validates monitor IDs against the live `mons` list, seamlessly adopting clients onto the primary display if their previous monitor is missing.
- **Root View Synchronization:** When restoring active tags from `_DWM_ACTIVE_TAGS`, out-of-range views cleanly fall back to tag 1, keeping each monitor's `pertag` structure synchronized.
- **Formally Verified in Lean 4:** Proven in `DwmState.lean` via `bv_decide` (roundtrip bijectivity on matching configurations and infallible non-zero invariant under arbitrary configuration mismatches).

---

## 10. ICCCM Size Hints & Exact Aspect Ratio Projection

Replaces stock dwm's sequential float division with an exact 64-bit cross-multiplied integer projection engine:

- **Exact Integer Aspect Ratio:** Aspect ratio boundaries ($w / h \le \text{max\_ax} / \text{max\_ay}$) are evaluated directly with 64-bit integer cross-multiplication (`w * max_ay <= h * max_ax`), eliminating IEEE 754 float precision loss and rounding jitter.
- **Strict Projection Idempotence ($f(f(r)) = f(r)$):** Unlike stock dwm where increment snapping after aspect adjustments pushes windows back outside legal aspect ratios, the coupled projection engine aligns to the increment lattice and stabilizes in a single pass. Calling `applysizehints` twice on any rectangle is guaranteed to be a no-op identity.
- **Zero ConfigureRequest Loops:** Eliminates infinite resize ping-pong loops between dwm and client windows (e.g. mpv, terminal emulators, Wine) that react to `ConfigureNotify` events.
- **Formally Verified in Lean 4:** Proven in `DwmHints.lean` via `omega` (aspect inequality satisfaction, increment alignment, and strict bounds safety).

---

## 11. Cycle-Safe Modal Z-Order Tree (`restack`)

Upgrades stock dwm's flat floating list scan to a fuel-bounded topological Z-order sorter for `WM_TRANSIENT_FOR` modal dialogs:

- **Modal Dialog Visibility:** When focusing or clicking on a parent window, dwm automatically raises all of its visible modal dialogs (and nested dialog chains $A \to B \to C$) strictly above the parent. Eliminates application UI freeze/lockups where parents bury their own modal dialogs.
- **Topological Sorting:** Floating windows are topologically sorted before emitting `XRaiseWindow()`, guaranteeing that children are always stacked higher than their ancestors.
- **Cycle-Safe Pointer Traversal:** Traversal uses a bounded fuel counter (`fuel = 64`), terminating in $O(1)$ constant time even if buggy or malicious X11 clients set circular `WM_TRANSIENT_FOR` properties ($A \to B \to A$).
- **Formally Verified in Lean 4:** Proven in `DwmTrans.lean` via structural induction on fuel (guaranteed termination on cyclic graphs and topological dominance $Z(\text{parent}) < Z(\text{child})$).

---

## 12. UTF-8 & Status2D Markup Boundary Sanitizer

Protects the asynchronous status engine against multi-byte sequence slicing and Status2D tag hijacking:

- **UTF-8 Continuation Boundary Rewind:** Validates continuation bytes `(b & 0xC0) == 0x80` and sequence lengths before string null-termination. If a 3- or 4-byte Nerd Font glyph is truncated by pipe buffer boundaries, the cursor rewinds at most 3 bytes to the preceding complete glyph. Eliminates malformed characters that cause `XftTextExtentsUtf8` and pill hitboxes to desync.
- **Status2D Delimiter Sanitizer:** Automatically tracks open caret tags (`^...^`). If a script output truncates mid-escape (e.g. `^c#ff5`), the sanitizer rewinds to the opening `^` to discard the partial tag, preventing color code injection into adjacent blocks.
- **Inter-Block Style Isolation:** Automatically injects a `^d^` color reset barrier between status badges so custom block colors never bleed into adjacent blocks.
- **Formally Verified in Lean 4:** Proven in `DwmStatus.lean` via `bv_decide` (byte class disjointness, continuation mask invariants, and markup state isolation).

---

## 13. Overflow-Proof Multi-Monitor Voronoi Routing (`recttomon` & `INTERSECT`)

Eliminates coordinate integer overflow and resolves non-overlapping floating windows in multi-monitor setups:

- **64-Bit Intersection Arithmetic:** Replaces stock 32-bit `INTERSECT()` macro with `rect_intersect_area()` using 64-bit integer arithmetic (`int64_t`). Large coordinate offsets and large client geometries (e.g. $x \approx 30000, w \approx 40000$) no longer overflow signed 32-bit integer into negative values or misassign windows to random monitors.
- **Two-Tier Voronoi Dead-Space Routing:** When a floating window is dragged into the "dead space" of an asymmetric multi-monitor layout (e.g. L-shaped or mixed-resolution setups), overlap area evaluates to 0 for all monitors. Instead of arbitrarily dumping the window onto `selmon`, `recttomon()` falls back to a Manhattan box-distance metric (`rect_box_distance()`) that routes the window to the physically closest monitor.
- **Display Disconnect Clamping Rescue:** When a secondary monitor is unplugged (`updategeom()`), floating windows migrated to the remaining monitor are automatically clamped inside the active screen boundaries (`[wx, wx + ww - WIDTH(c)]`). Windows are never stranded invisibly off-screen in unmapped coordinate space.
- **Formally Verified in Lean 4:** Proven in `DwmVoronoi.lean` via `bv_decide` and `omega` (zero 64-bit overflow on all 16-bit X11 spans, Manhattan metric non-negativity $d \ge 0$, and guaranteed visibility span $\ge \min(w, ww)$ on monitor disconnect).

---

## 14. Passive Grab & Modifier Normalization Bijection (`grabkeys` vs. `keypress`)

Guarantees exact mathematical symmetry between passive X11 grabs and event dispatch:

- **Canonicalized Grab Registration:** Passes `CLEANMASK(keys[i].mod)` and `CLEANMASK(buttons[i].mask)` into `XGrabKey` / `XGrabButton` rather than raw uncleaned binding masks. Prevents accidental inclusion of `LockMask`, `numlockmask`, or invalid bits from corrupting the 4-permutation Cartesian product (`{0, LockMask, numlockmask, numlockmask|LockMask}`). The unmasked base shortcut is guaranteed to register.
- **Ghost Event Swallowing Elimination:** With canonicalized grabs, every hardware modifier state that activates an X11 root grab is guaranteed to be recognized and dispatched by `keypress()` / `buttonpress()`. No keys are swallowed silently by the window manager without firing an action.
- **Dynamic Modifier Map Synchronization:** `mappingnotify()` explicitly handles `MappingModifier` events in addition to `MappingKeyboard`. Re-evaluates `updatenumlockmask()` and re-registers all root grabs whenever `xmodmap`, `setxkbmap`, or desktop layouts modify keyboard modifier assignments.
- **Formally Verified in Lean 4:** Proven in `DwmGrab.lean` via `bv_decide` (canonicalizer idempotence, base key grab membership, soundness, and exact bijection between X11 grab filter and event dispatch).

---

## 15. Floating Bar Island & Pill Hit-Testing Partition (`drawbar` vs. `buttonpress`)

Unifies status measurement and mouse click routing into an exact geometric partition:

- **Zero-Drift Status Width:** Removed arbitrary $+2\text{px}$ offset in `status2d_textw()`, perfectly synchronizing measured string dimensions with the geometry accumulator in `drawstatusbar()`.
- **Per-Pill Hit-Testing:** Caches exact horizontal pill intervals (`m->bar_pills[]`) during status rendering. Clicks in `buttonpress()` are matched against pill intervals and passed via `arg.i = block_id` (1-based index), enabling block-specific click handlers.
- **Collision-Proof Component Clamping:** Status start is strictly clamped at $x_{\text{status}} \ge x_{\text{lt}} + w_{\text{lt}}$, preventing long status text or low-resolution viewports from overdrawing or masking layout symbol and tag clicks. Title/spacer rectangle calculation ($w = x_{\text{status}} - x$) never underflows.
- **Formally Verified in Lean 4:** Proven in `DwmBar.lean` via `omega` (pairwise interval disjointness, exact hit routing, and pill enclosure without component collision).
