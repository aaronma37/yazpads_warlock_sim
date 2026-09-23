# Classic WoW UI Transformation - Phased Implementation TODO

This document outlines a steady, step-by-step plan to transform our ImGui interface into an authentic Classic World of Warcraft UI.

---

## 🎯 Architecture Overview

Our application uses **Raylib + rlImGui + Dear ImGui** (with Emscripten / WebAssembly support). 
Blizzard UI assets (PNG textures) are located at `~/wow_assets/wow-ui-textures/` and can be loaded via `AssetManager`.

```
                  ┌────────────────────────────────────────┐
                  │           Classic WoW UI Layer         │
                  └───────────────────┬────────────────────┘
                                      │
        ┌─────────────────────────────┼─────────────────────────────┐
        │                             │                             │
┌───────▼───────────┐       ┌─────────▼──────────┐       ┌──────────▼───────────┐
│  WoW Nine-Slice   │       │  WoW Widget Set    │       │  WoW Theme & Fonts   │
│  & Texture Engine │       │  (Buttons, Tabs,   │       │  (Friz Quadrata,     │
│  (ImDrawList)     │       │   Scrollbars, etc) │       │   Morpheus, Gold Plt)│
└───────────────────┘       └────────────────────┘       └──────────────────────┘
```

---

## 📋 Phased Roadmap

### Phase 1: Typography & Color Palette (Immediate Win) ✅
- [x] **Acquire & Bundle Fonts**:
  - `FRIZQT__.TTF` (Primary titles, headers, button labels - configured with fallback lookups)
  - `MORPHEUS.TTF` / `MedievalSharp` (Subtext, body, lore, stats)
  - `SKURRI.TTF` / `Cinzel-Bold` (DPS / Combat numbers)
- [x] **Load Font Atlas in `rlImGui` / ImGui IO**:
  - Registered `load_wow_fonts()` via `rlImGuiSetLoadFontsCallback()`.
  - Configured `Regular` (14px), `Header` (18px), `Small` (12px), `Flavor` (15px), and `Combat` (16px).
- [x] **Update Color Palette in `ui_theme.hpp`**:
  - Base Gold: `#FFD100` (`ImVec4(1.0f, 0.82f, 0.0f, 1.0f)`)
  - Highlight Gold: `#FFE680`
  - Parchment Body Text: `#EBD9B8`
  - Dark Charcoal Stone Frame & Slate Backgrounds
  - WoW Item Quality Colors (Poor, Common, Uncommon, Rare, Epic, Legendary, Artifact).
  - Class Colors (Warlock, Priest, Mage).

---

### Phase 2: Core Texture Rendering Primitives (The Engine) ✅
- [x] **Implement 9-Slice Renderer (`DrawNineSlice`)**:
  - Implemented in `src/ui/common/wow_widgets.hpp` with 9 quad subdivision, proportional corner scaling for small bounding boxes, and UV coordinates support.
- [x] **Implement 3-Slice Horizontal & Vertical Helpers**:
  - `DrawThreeSliceHorizontal` (left cap, middle stretch, right cap) for standard buttons & headers.
  - `DrawThreeSliceVertical` (top cap, middle stretch, bottom cap) for scroll tracks and columns.
  - `DrawTiledTexture` for seamless repeating stone/parchment backgrounds without distortion.
- [x] **Texture Asset Ingestion**:
  - Enhanced `AssetManager` with recursive indexing of `~/wow_assets/wow-ui-textures/` (DialogFrame, Buttons, FrameGeneral, Tooltips, etc.).
  - Added `get_texture()` helper supporting flexible path resolution and lowercase stem lookups.

---

### Phase 3: Custom Window Frame & Container Panels ✅
- [x] **Classic WoW Dialog / Parchment Windows**:
  - Implemented `DrawWowDialogBackdrop()` to render seamless repeating stone tiling and antique borders on windows and canvases.
- [x] **Dialog Header Medallions / Title Bars**:
  - Implemented `DrawWowSectionHeader()` utilizing `UI-DialogBox-Header.PNG` with centered Friz Quadrata gold text.
- [x] **Close Button ("Red X")**:
  - Implemented `WowCloseButton` with `UI-Panel-MinimizeButton-Up`, `-Down`, and `-Highlight`.
- [x] **Inset Frames / Sub-panels**:
  - Implemented `BeginWowChild()` / `EndWowChild()` with inset stone well styling and borders across Preset Panes.

---

### Phase 4: Custom WoW Widgets ✅
- [x] **Classic Action & Panel Buttons (`WowButton`)**:
  - 3-slice horizontal sprite rendering with `UI-Panel-Button-Up`, `Down`, `Disabled`.
  - Additive gold highlight glow on hover with `UI-Panel-Button-Highlight`.
  - (+1px, +1px) text displacement when active/held.
  - Applied to simulation run triggers, optimizer controllers, and export actions.
- [x] **Checkboxes (`WowCheckbox`)**:
  - Classic stone square check frame (`UI-CheckBox-Up` / `Down`) + gold checkmark (`UI-CheckBox-Check`) and hover highlight.
- [x] **Scrollbars & Sliders**:
  - Styled with antique brass thumbs and dark stone tracks.
- [x] **Tabs (Stone Arch Tabs)**:
  - Themed top-level and sub-level tabs with dark slate and gold accents.

---

### Phase 5: Polish & Game Immersion ✅
- [x] **Item & Spell Slot Borders**:
  - Implemented `DrawWowItemSlot()` with authentic Blizzard action borders (`UI-ActionButton-Border.PNG`, `UI-EmptySlot-White.PNG`) tinted by Item Quality (Common, Uncommon, Rare, Epic, Legendary).
  - Added square action gloss highlight overlay (`ButtonHilight-SquareQuickslot.PNG`) on item hover.
- [x] **WoW Tooltip Frame**:
  - Implemented `DrawWowTooltipBackdrop()` with black marble background (`UI-Tooltip-Background.PNG`) and 9-slice gold border (`UI-Tooltip-Border.PNG`).
  - Added color-coded stat formatting in item tooltips with `wow_colors` palette.
- [x] **Plot & Graph Styling (ImPlot)**:
  - Configured dark obsidian parchment background, antique brass plot borders, gold plot lines, and class-colored data traces in `apply_wow_theme()`.
- [x] **Performance & Reliability Verification**:
  - Validated native builds and 156/156 unit test suite passing cleanly with zero regressions.

---

## 📁 Key File Mapping

| Component | Target File |
|---|---|
| Theme & Colors | [`src/ui/common/ui_theme.hpp`](file:///home/deck/warlock_sim/src/ui/common/ui_theme.hpp) |
| Texture & Font Loading | [`src/ui/common/asset_manager.hpp`](file:///home/deck/warlock_sim/src/ui/common/asset_manager.hpp) / [`src/ui/ui_app.hpp`](file:///home/deck/warlock_sim/src/ui/ui_app.hpp) |
| 9-Slice & Widget Helpers | `src/ui/common/wow_widgets.hpp` *(New)* |
| Window & Panel Integrations | `src/ui/common/` and `src/ui/warlock/`, `src/ui/priest/` |
