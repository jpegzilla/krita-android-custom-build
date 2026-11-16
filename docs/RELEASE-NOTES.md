# Release notes and historical status

Chronological notes for the customization layers, moved out of the README.
These are historical claims, not current build status. For the exact build of a
commit use the local watcher result (Issue #1) and
[DEVELOPMENT_STATE.md](DEVELOPMENT_STATE.md).

## v1

Compiled and tested; working one-finger double-tap Mirror View and narrow
Layers docker.

- Successful Android debug build (`v1 / run #4`).
- Generated as `krita-arm64-v8a-5.3.4-...-debug.apk`, intended to install as a
  separate app.

## v2

Compiled locally into an APK; user confirmed Compact UI, one-shot picker,
reference sampling, native volume Undo/Redo, gallery and key controls largely
work.

Source changes:

- **Compact UI:** hides the menu bar, document tabs and bottom status bar
  without changing Android's immersive mode or the arrangement of current
  dockers/toolbars; toggled from the top toolbar.
- **Quick Actions** and common actions in the default top toolbars: Save,
  Deselect, selected-layer Alpha Lock, reference visibility, Reload Preset,
  reset canvas view, one-shot color picker and sampling-source toggle. Brush
  size and opacity controls remain.
- **Tablet Controls dialog** to adjust double-tap delay and optional
  quick-pinch reset timing; enable/disable volume-button undo/redo.
- **Volume Down = Undo; Volume Up = Redo**, handled natively in the Android
  activity with a configurable on/off switch.
- **One-shot color picker:** use the new toolbar action to switch to sampler for
  one pen press, then return to the freehand brush. **Sample Visible Layers**
  toggle currently applies to the dedicated color-sampler tool.
- **Direct pen-tap color pickup from visible reference images**, intended to
  avoid a brush stroke when the pen touches a reference.
- **Optional quick inward pinch reset**, initially **off** to avoid gesture
  conflicts; normal pinch gestures retained. Uses existing Fit Canvas and Reset
  Rotation actions.
- **Selection Actions Bar** enlarged on Android, enabled and anchored at the
  bottom in Compact UI; restored to the user's original selection-bar settings
  when exiting.
- **Welcome screen:** replaces news column with more space; integrates gallery
  in the Recent Images area with a toggle, sorting and local-folder selection.
  Recent-document content URIs remain a fallback.

Deferred at the time: finger-dragging reference images with any tool; complete
Android SAF folder traversal for files saved by other apps; a transform-specific
bottom action bar; automatic adaptation of every Tool Options control at
extremely narrow docker widths; and an exact new default workspace matching an
image (existing user workspace settings are preserved).

## v3 refinements

Code written in `tools/patches/apply_tablet_refinements.py`; source patch
validation passed. Not confirmed as a new APK at the time; v2 remained the last
verified build.

- Compact UI attempts to reclaim hidden document-tab height instead of leaving
  a gray strip; hides canvas and MDI scrollbars and restores their previous
  policies on exit.
- Default top toolbar is icon-dense instead of large text labels. Quick Actions
  opens by tapping the entire button. Removes a duplicate brush preset reset
  button; narrows the brush blending-mode selector.
- Moves Gradient and Pattern selectors to Quick Actions. Quick Actions is now
  configurable via a checklist and reorderable list.
- One-finger double-tap defaults to Mirror View in Krita Default input profile
  on new profiles. The interval defaults to **150 ms**; previously stored
  nondefault intervals remain unchanged. One-shot sampler gets
  **Ctrl+Alt+Shift+P** by default.
- Quick two-finger inward pinch reset defaults to **enabled** for users without
  a saved preference; timing remains configurable.
- Native volume key gestures: separate short/long bindings for both volume
  buttons. Default: Down short Undo, Up short Redo, Down long Pick Color Once,
  Up long Toggle Eraser. Long-press threshold defaults to **150 ms**. Normal
  actions trigger on key release; brush size Up/Down may be selected as a
  repeating action while held.
- Direct finger dragging of visible reference images from any tool; movement is
  recorded as an undoable shape command. Can be disabled in Tablet Controls.
- Selection Actions Bar enlarged with additional selection-relevant actions.
  Transform tool gets its own bottom contextual bar (mode, reset, cancel,
  apply) in Compact UI.
- Gallery defaults to shared Downloads and has a Downloads button plus
  **opt-in All Files Access** settings route. Android permission must be
  explicitly granted; the permission does not allow access to other apps'
  private Android/data files. Gallery rename/move/advanced organization is
  still deferred.
- More flexible narrow Tool Options layout constraints. Individual tool widgets
  may still require specialized responsive work.

## v4 UI correction patch

Implemented in `tools/patches/apply_tablet_polish.py`, after v3. Source
validated; APK not built at the time.

- **Compact UI:** Qt QMdiArea still reserved its document tab height after
  simply hiding QTabBar. v4 switches to frameless MDI subwindows only while
  Compact UI is active, then restores the original MDI view mode and document
  tab styling when switched off. This does not change Android
  immersive/fullscreen state. Standard document tabs should become visible
  again, including the filename and unsaved marker.
- **Welcome:** Hide the image-specific toolbar buttons on the welcome screen,
  and restore the toolbar when a document opens.
- **Icons:** Sample Visible Layers gets a distinct icon from Pick Color Once;
  reference visibility gets a compact reference icon instead of long text. The
  brush blend-mode selector maximum width is lowered from 116 to 90.
- **Volume settings:** Remove redundant short/long enable checkboxes. Keep one
  master volume-key toggle and the four individually assignable actions (each
  can be `None`). Long-press threshold remains configurable.
- **Double-tap:** Assign Mirror View (150 ms default) to the saved/current input
  profile on a one-time migration **only when the gesture is not already
  bound**. A customized double-tap is preserved.
- **Reference image:** Add two-finger move, resize and rotate gestures with a
  single undoable shape-transform command. Experimental.
- **Deferred:** Screen-anchored/pinned reference overlays and a complete
  relocation of every special tool's options into the bottom strip. Transform
  and selection context bars remain enabled.

Validation caveat: the GitHub workflow checks patch application and whitespace,
not C++ or Java compilation.

## v7 Compact UI phone layout

Uses `mobile_studio/include/mobile_studio/tablet_compact_skin.h` and
`tools/patches/apply_compact_skin.py`. All new controls appear **only when
Compact UI is enabled**; on exit, Krita's QMainWindow docker state is restored.

- **Paint / Erase / Blend slots** (bottom-left): first tap switches to the
  last-used native Krita brush preset for that category; second tap opens a
  touch-oriented icon grid of presets. Each category remembers its last preset
  independently, including after restarts. Paint disables eraser mode; Erase
  enables it; Blend selects a normal Krita smudge/blender preset with eraser
  mode off. This is *not* a universal conversion of all paint engines into
  smear engines.
- Presets are initially grouped by names (erase/eraser, blend/smudge/blur;
  others Paint), with user overrides stored privately in `TabletUi/presetGroup`.
  In the preset grid, enable **Show all Krita brushes** and choose a brush to
  put it in a different group; **Assign current brush to this category** is
  another option. Built-in Krita preset tags and the native brush selector
  remain unchanged.
- **Layer thumbnails rail** (left): scrollable thumbnail list of top-level
  layers; tap to select, tap the selected layer again to open a compact controls
  panel. It offers blending mode, opacity, visibility, duplicate, clear, lock,
  merge, delete, clipping group, opaque selection and transparency mask,
  delegating to Krita's existing commands where possible. An Add button creates
  a paint layer. It shows an abbreviated active-layer name.
- **Color selector** (bottom-right): Krita's existing Advanced Color Selector
  docker is reused as a resizable floating wheel. Tap the color button to
  minimize/restore it; adjacent small button cycles three wheel sizes; recent
  color swatches remain in a narrow row. Original docker layout is restored on
  exiting Compact UI.
- **Top popup buttons:** native full brush preset chooser and Tool Options.
  When Tool Options are in docker mode, its button shows the existing docker; it
  does not forcibly rewrite Krita's global preference.
- Limitations: only top-level layers are shown in the first experimental rail,
  and color docker floating behavior on Android remains untested. There is no
  automatic conversion of any arbitrary brush to a true smudge engine.
