# C++ Crosshair Overlay

A small Windows crosshair overlay written in C++ with the Win32 API.

Current app version: `v1.0.1`.

## Build

Easiest option:

```powershell
.\build.ps1
```

The helper uses CMake, MSVC, or MinGW if one is installed.

Manual CMake build:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Run it with:

```powershell
.\build\Release\crosshair.exe
```

If your CMake generator does not use `Release` folders, the executable may be at:

```powershell
.\build\crosshair.exe
```

The direct MSVC helper output is:

```powershell
.\build\msvc\crosshair.exe
```

The direct MinGW helper output is:

```powershell
.\build\mingw\crosshair.exe
```

This workspace also contains a portable build output after running the helper:

```powershell
.\dist\crosshair.exe
```

Use `crosshair.exe` for normal 64-bit Windows. If `crosshair-x86.exe` exists, keep it only as a fallback for 32-bit Windows.

## Controls

- `Ctrl+Alt+S` opens the fullscreen overlay editor.

The overlay editor appears as a borderless topmost HUD with a transparent black backdrop, rounded floating panels, and rounded action buttons spread across the screen, closer to a Steam/Xbox-style overlay than a normal app window. Drag a panel background to move it. Press `Esc` or `Hide` to close the editor, or use `Exit` to close the whole app.

The editor customizes length, gap, thickness, opacity, position offset, full RGB color, center dot, outline, and visibility. Sliders and typed number boxes stay synced, so you can drag quickly or enter exact values.

The preset dropdown applies complete crosshair styles, including shape, opacity, RGB color, dot, outline, and offset. Settings are saved automatically.

Use `Save preset` to store the current crosshair as a custom preset. Select a custom preset and use `Remove` to delete it.

You can also right-click the tray icon for common actions and exit.

## Updates

The app checks `VKnubis/crossair` on GitHub in the background when it starts. It only shows a popup if it finds a newer release or version tag.

You can manually check from the tray menu or the `Updates` button in settings. When a newer release includes a `.exe` asset, the updater can download it, close the running app, replace the current executable, and restart Crosshair automatically. Publish GitHub releases or tags like `v1.0.1` for the checker to recommend updates.

Use overlays only in apps and games where they are allowed.
