# C++ Crosshair Overlay

A small Windows crosshair overlay written in C++ with the Win32 API.

Current app version: `v1.1.0`.

## Build

Build with the provided PowerShell script:

```powershell
.\build.ps1
```

This compiles using the bundled LLVM MinGW toolchain and produces:

```powershell
.\dist\crosshair.exe       # 64-bit executable
.\dist\crosshair-x86.exe   # 32-bit executable (optional fallback)
```

Run the 64-bit version:

```powershell
.\dist\crosshair.exe
```

## Controls

- `Ctrl+Alt+S` opens the settings editor (can be customized)
- `Ctrl+Alt+H` closes the settings editor (can be customized)
-`Ctrl+Alt+S+f12` for fall back opener
- `Esc` also closes the editor
- Right-click the tray icon for menu options (Toggle, Longer, Shorter, Color Cycle, Help, Exit)

The settings editor displays as a fullscreen overlay with draggable panels for Settings, Presets, Shape, Color, Options, and Actions. You can customize:

- **Length**: 4-80 pixels
- **Gap**: 0-40 pixels
- **Thickness**: 1-10 pixels
- **Opacity**: 30-100%
- **Position Offset**: -200 to +200 pixels (X and Y)
- **Color**: Full RGB control
- **Center Dot**: Toggle on/off
- **Outline**: Toggle on/off
- **Visibility**: Toggle on/off

Sliders and input fields stay synced—drag for quick adjustments or enter exact values. Drag any panel by its background to reposition it.

**Custom Presets**: Save your current crosshair settings as a preset using the "Save preset" button. Select and apply saved presets from the dropdown, or remove them with the "Remove" button. Settings persist automatically.

## Updates

The app checks `VKnubis/crossair` on GitHub in the background when it starts. It only shows a popup if it finds a newer release or version tag.

You can manually check from the tray menu or the `Updates` button in settings. When a newer release includes a `.exe` asset, the updater can download it, close the running app, replace the current executable, and restart Crosshair automatically. Publish GitHub releases or tags like `v1.0.1` for the checker to recommend updates.

Use overlays only in apps and games where they are allowed.
