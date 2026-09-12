# LogiOps GUI Customizer

A graphical, user-friendly interface built in Python (Tkinter) for configuring Logitech mice and keyboards managed by the [`logiops`](https://github.com/PixlOne/logiops) (`logid`) daemon on Linux.

---

## Table of Contents
1. [Overview](#overview)
2. [Prerequisites & Dependencies](#prerequisites--dependencies)
3. [Installation & Setup](#installation--setup)
4. [Launching the Application](#launching-the-application)
   - [From Application Launcher](#from-application-launcher)
   - [From Terminal](#from-terminal)
5. [Features & Capabilities](#features--capabilities)
   - [Device Auto-Detection](#device-auto-detection)
   - [Status & Help Dashboard](#status--help-dashboard)
   - [Pointer & Scrolling](#pointer--scrolling)
   - [Custom Button Mapping & Gestures](#custom-button-mapping--gestures)
   - [Side Thumbwheel Mapping](#side-thumbwheel-mapping)
   - [Configuration Persistence & Preview](#configuration-persistence--preview)
6. [Key Fixes & Technical Enhancements](#key-fixes--technical-enhancements)
7. [Systemd Service Resilience](#systemd-service-resilience)
8. [Troubleshooting Guide](#troubleshooting-guide)

---

## Overview

The `logiops` daemon configures Logitech HID++ devices via `/etc/logid.cfg`. While powerful, configuring it manually requires knowledge of raw kernel keycodes, CIDs, libconfig syntax, and manual daemon restarts.

`logiops_gui.py` provides an intuitive graphical interface to configure all features, monitor mouse health, view live status, preview changes, and automatically update `/etc/logid.cfg` and restart the daemon.

---

## Prerequisites & Dependencies

### System Dependencies
To build and run `logiops` and the GUI on Ubuntu/Debian:
```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libevdev-dev libudev-dev libconfig++-dev libglib2.0-dev python3-tk
```

---

## Installation & Setup

1. **GUI Script Location**:
   The script is located at:
   ```bash
   ~/logiops_gui.py
   ```

2. **Desktop Launcher Entry**:
   A `.desktop` launcher is installed at:
   ```bash
   ~/.local/share/applications/logiops-gui.desktop
   ```
   This allows launching directly from the GNOME / Ubuntu Application Launcher.

---

## Launching the Application

### From Application Launcher
1. Press `Super` (Windows key) to open the Application Launcher.
2. Search for **LogiOps Customizer**.
3. Click the icon and enter your administrator password in the graphical authentication prompt (`pkexec`).

### From Terminal
Because `logid` requires administrative privileges to edit `/etc/logid.cfg` and manage the systemd service, run:
```bash
sudo python3 ~/logiops_gui.py
```

---

## Features & Capabilities

### Device Auto-Detection
- Automatically searches recent system logs (`journalctl -u logid`) and kernel `sysfs` power supplies for detected device names (e.g. `MX Master 3S`).
- Includes an **Auto-Detect** button to refresh the active device name at any time.

### Status & Help Dashboard
- **Live Service Status**: Displays whether the `logid` systemd service is active or inactive.
- **Device Connection**: Queries the `pizza.pixl.LogiOps` D-Bus tree to monitor device connectivity.
- **Battery Status**: Reads device battery percentage and charging state directly from `/sys/class/power_supply/`.
- **Fix 'double run' Button**: Instantly terminates duplicate/zombie `logid` background processes that cause `IOMonitor double run` crashes and restarts the service cleanly.
- **Custom App Launching Guide**: Step-by-step instructions for triggering custom Linux applications (e.g., terminal, Spotify, custom scripts) via mouse buttons.
- **Terminology & Feature Glossary**: Built-in interactive documentation explaining DPI, SmartShift, Threshold, Torque, HiRes Scroll, and Gestures.

### Pointer & Scrolling
- **DPI Setting**: Real-time slider from 200 to 4000 DPI.
- **SmartShift (Freespin)**:
  - Toggle SmartShift on/off.
  - **Threshold** (1–255): Speed required to release the wheel into free-spin.
  - **Torque** (1–255): Resistance/tactile stiffness of ratchet mode.
- **HiRes Scrolling**:
  - High-resolution smooth scrolling toggle.
  - **Invert Scroll**: Natural scrolling reversal.
  - **Target**: Routes high-res scroll events directly to the target window.

### Custom Button Mapping & Gestures
Supports dedicated configuration tabs for:
- **Thumb Button** (`0xc3`)
- **Top Button** (`0xc4`)
- **Forward Button** (`0x53`)
- **Back Button** (`0x56`)

For each button, configure:
1. **Single Press (No Swipe)**: Triggered upon clicking without mouse movement.
2. **Gestures (Swipes)**: Up, Down, Left, and Right swipes while holding the button.
3. **Action Types**:
   - `Do Nothing` (`NoPress` or pass-through)
   - `Keypress`: Key combinations with pre-populated presets (Print Screen `KEY_SYSRQ`, Super `KEY_LEFTMETA`, Copy/Paste, Media Play/Pause, Volume, etc.) or custom keys.
   - `ToggleSmartshift`: Toggles wheel between ratchet and free-spin.
   - `CycleDPI`: Cycles through custom DPI levels (e.g. `400, 800, 1200, 1600`).

### Side Thumbwheel Mapping
Dedicated tab for horizontal scroll wheels (such as on the MX Master 3 / 3S):
- **Divert**: Intercepts raw horizontal scroll events.
- **Invert**: Inverts scroll direction.
- **Scroll Left & Scroll Right Remapping**:
  - Remap to volume controls (`KEY_VOLUMEDOWN` / `KEY_VOLUMEUP`), track navigation, or any custom keypress.
  - Configured with `interval: 1;` mode for reliable interval-based event triggering.

### Configuration Persistence & Preview
- **Bidirectional Config Parsing**: Automatically parses `/etc/logid.cfg` on startup using balanced block extraction so existing configurations are never overwritten or lost.
- **Preview Configuration Diffs**: View the full generated `/etc/logid.cfg` before saving.
- **Copy to Clipboard**: Quick button inside the preview window to copy the raw configuration text.
- **One-Click Save & Apply**: Saves to `/etc/logid.cfg` and immediately restarts `logid.service`.

---

## Key Fixes & Technical Enhancements

1. **`IOMonitor double run` Prevention**:
   - Resolved conflicts where manual `sudo logid -v` commands caused daemon crashes due to simultaneous device access.
   - Added automated cleanup routines via the GUI.

2. **Polkit (`pkexec`) Graphical Environment Forwarding**:
   - Resolved silent GUI launch failures when run from desktop launcher by passing current display variables:
     ```ini
     Exec=sh -c "pkexec env DISPLAY=$$DISPLAY XAUTHORITY=$$XAUTHORITY python3 /home/sigmind/logiops_gui.py"
     ```

3. **Thumbwheel `interval: 1;` Syntax Fix**:
   - Fixed side thumbwheel actions failing to fire by correcting the generated config from `pixels: 1;` to `interval: 1;`.

4. **Regex-Based Smart Keycode Extraction**:
   - Accepts human-readable labels (e.g. `Volume Up (KEY_VOLUMEUP)`) while accurately extracting kernel keycodes (`KEY_VOLUMEUP`) into the configuration file.

---

## Systemd Service Resilience

To ensure `logid` starts on boot and automatically restarts if it encounters errors or crashes:

1. **Service Override**:
   Location: `/etc/systemd/system/logid.service.d/override.conf`
   ```ini
   [Service]
   Restart=on-failure
   RestartSec=5s
   ```

2. **Apply & Verify**:
   ```bash
   sudo systemctl daemon-reload
   sudo systemctl restart logid
   sudo systemctl status logid
   ```

---

## Troubleshooting Guide

| Issue | Cause | Solution |
|---|---|---|
| **App closes after entering password** | `pkexec` stripped `DISPLAY` / `XAUTHORITY`. | Verify `~/.local/share/applications/logiops-gui.desktop` uses the `sh -c "pkexec env ..."` wrapper. |
| **Service status Inactive / Failed (`ABRT`)** | `IOMonitor double run` crash from an existing `logid` process. | Click **Fix 'double run'** in the Status tab, or run `sudo killall logid && sudo systemctl restart logid`. |
| **Thumbwheel volume not changing** | Missing `interval: 1;` parameter or unconfigured actions. | Update GUI to latest version, enable Side Thumbwheel, select `Keypress` -> `Volume Up/Down`, and save. |
| **Settings reset after editing DPI** | Config file wasn't reloaded on startup. | The app now automatically loads existing configurations from `/etc/logid.cfg` on launch. |
| **Device shows Disconnected / Asleep** | Normal D-Bus behavior when device is idle. | Move the mouse or click a button to wake up communication. |

---

*Authored for Logitech Linux Configuration Daemon (`logiops` / `logid`).*
