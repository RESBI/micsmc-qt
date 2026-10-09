# micsmc-qt — Qt6 control panel

**English** | [简体中文](README_CN.md)

A Qt6 reimplementation of Intel MPSS's `micsmc-gui` control panel: a native desktop
application that follows the panel's layout and colour scheme (menu bar / overview
panel / per-card view / right-hand field widgets / settings dialog).

- **Qt6 Widgets only** — no qt6-charts; the line chart, histogram and 7-segment
  temperature display are all custom-painted with QPainter
- **Sampling runs on a worker thread** — every `mic_*` call happens inside a QThread,
  and the UI thread only receives queued signals
- Bilingual UI (follows the system locale by default, switchable from the Advanced menu)
- Card selection, Turbo / LED control, and an Advanced Settings dialog

## Screenshot

![Overview panel, Utilization View (All Devices)](images/screenshot-utilization-view.png)

The overview panel (`Utilization View (All Devices)`) on a single-card system
(`Cards: 1  Memory: 15 GB  Cores: 61`): legend and 60-second curves on the left;
digital temperature readout, memory bar and power bar on the right. The core
temperature here is 86 °C, past the first threshold (85 °C by default), so the
readout is rendered in the **yellow** alert colour. See §5.4 and §6 for the
three-level alert behaviour (yellow / orange / red) and how to configure it.

---

## 1. Requirements

### 1.1 Build-time

| Dependency | Notes | Install on AOSC OS |
|---|---|---|
| CMake | ≥ 3.16 | `sudo pacman -S cmake` |
| C++ compiler | C++17 (gcc or clang) | `sudo pacman -S gcc` |
| Qt6 Base | provides `Qt6::Widgets` (qt6-base) | `sudo pacman -S qt6-base` |
| libmicmgmt | provides `miclib.h` and `libmicmgmt.so` (MPSS `04-mpss-micmgmt`) | installed with MPSS |
| libscif | provides `libscif.so` (MPSS `02-libscif`) | installed with MPSS |

Qt is located with `find_package(Qt6 ...)`; the MPSS libraries are linked directly
with `-lmicmgmt -lscif` (they ship no pkg-config files, so the libraries must be on
the default search path). `miclib.h` is installed to `/usr/include` by
mpss-micmgmt.

### 1.2 Run-time

- The kernel `mic` driver is loaded (`lsmod | grep mic`) and MPSS is up — `micctrl
  --status` should list your cards
- At least one Intel Xeon Phi (KNC) coprocessor
- A graphical session (X11 or Wayland); for remote use see §4.3
- **Changing card settings requires root**; monitoring alone works as a normal user

---

## 2. Building

Run the following from the project root (the directory containing this README):

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

The binary is written to `build/micsmc-qt`.

Useful CMake options:

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..        # optimised build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..       # install prefix (default: /usr/local)
```

---

## 3. Installing and uninstalling

```bash
cd build
sudo make install      # installs to /usr/local by default
sudo make uninstall    # removes exactly what was installed
```

| Installed item | Path (default prefix `/usr/local`) |
|---|---|
| Executable | `<prefix>/bin/micsmc-qt` |
| Desktop entry | `<prefix>/share/applications/micsmc-qt.desktop` |

To use a different prefix, pick either form:

```bash
cmake -DCMAKE_INSTALL_PREFIX=/usr .. && sudo make install   # at configure time
sudo cmake --install . --prefix /opt/micsmc                 # one-off override
```

Staged installs (`DESTDIR`) are supported too:

```bash
DESTDIR=/tmp/pkg make install
DESTDIR=/tmp/pkg make uninstall
```

How uninstall behaves (see `cmake_uninstall.cmake.in` for the details):

1. **Paths come from `install_manifest.txt`** — the real absolute paths recorded at
   install time. Installing with a `--prefix` override therefore still uninstalls
   from the right locations.
2. **Allow-list guard**: only `micsmc-qt` and `micsmc-qt.desktop` are ever removed.
   Any other entry found in the manifest is skipped with a warning.
3. **Files only, never directories** — so shared directories such as `/usr/bin` and
   `/usr/share/applications` cannot be wiped along with other packages' files
   (`REMOVE_RECURSE` would do exactly that). Empty directories may be left behind;
   remove them by hand if you care.

---

## 4. Running

### 4.1 Without installing

```bash
cd build && ./micsmc-qt
```

### 4.2 Installed

```bash
micsmc-qt                      # from a terminal
# or launch "Xeon Phi Control Panel" from your desktop's application menu
```

### 4.3 Over SSH

This is a native GUI, so the display has to be forwarded:

```bash
ssh -X user@host micsmc-qt      # X11 forwarding
# alternatively, log into the desktop over VNC / RDP and start it locally
```

> Curve redraws are sensitive to network latency over X11 forwarding; for a smooth
> refresh rate, prefer VNC / RDP and run the app on the machine itself.

### 4.4 Optional environment variables

| Variable | Effect |
|---|---|
| `MICSMC_QT_THEME=dark` | Use the dark theme instead of the default light one |

---

## 5. Using the application

The window is laid out top to bottom in four bands: **menu bar → overview panel →
per-card view (appears once a card is selected) → status bar**. Data refreshes once
per second.

### 5.1 Menu bar

| Control | Action |
|---|---|
| `Cards ▾` | Lists every card in the system (`mic0`, `mic1`, …); `Show All` expands the card view, `Hide All` collapses it |
| `Advanced ▾` | `Error Log` / `Advanced Settings…` / `Update Application` / `Info` / `About…`; the last entry switches between Chinese and English |
| Right-hand `Cards: N  Memory: xx GB  Cores: N` | Card count / total memory / total cores |

### 5.2 Overview panel (`Utilization View (All Devices)`)

- **Legend**: `System [%]` (red) and `User [%]` (violet), matching the two curves below
- **Chart**: the last **60 seconds** as a time series, with a Y axis on both sides
  (0–100) and timestamps along X; the X axis is titled `Average Core Utilization`
- **Temperature field**: a 7-segment readout of the core temperature (°C), labelled
  `Average Core / Temperature °C` underneath
- **Memory field**: used memory in GB above, a vertical progress bar in the middle,
  `Total Memory Usage` below
- **Power field**: instantaneous power in Watts above, a vertical progress bar in the
  middle, `Total Power Usage` below

### 5.3 Per-card view (expands after picking a card from `Cards ▾`)

Selecting a card grows the window downwards and adds a panel titled
`mic0: Utilization View`. Its structure mirrors the overview panel, but the data
covers only that card (the temperature label becomes `Processor Core`, and memory
and power are per-card values).

The **two small buttons in the panel's top-right corner** switch views:

| Button | View |
|---|---|
| ▤ | `Utilization View` (default; time-series curves) |
| ▥ | `Core Histogram View` (one bar per core; hover shows `Core N: x%`) |

### 5.4 Alerts

- **Three-level temperature overlay** — the 7-segment readout itself changes colour:
  yellow above `coreTempLimit` (**85 °C** by default), orange above `coreTempCaution`
  (default +10), red above `coreTempCritical` (default +20)
- **Power bar zoning** — the portion of the bar above each device-reported setpoint is
  tinted yellow / orange / red in turn (Low / High / Shutdown setpoint; the red zone
  starts once power is within 10 % of the shutdown limit)
- The status bar reports the outcome of actions and any error messages

### 5.5 Changing card settings

`Advanced ▾ → Advanced Settings…`:

- **Card Settings** — one checkbox column per card
  - `Turbo` / `LED`: take effect as soon as you press `OK` (no card restart needed)
  - `ECC`: **greyed out, not implemented** — it requires the full maintenance-mode
    sequence (enter maintenance mode → apply → poll progress → exit)
- **Card Connections** — online state of each card
- **Log File** — placeholder note (log rotation is handled by mpssd and is not wired
  into this application)

> Settings changes need root; as a normal user the dialog is read-only.

---

## 6. Runtime configuration

| Item | Notes |
|---|---|
| Config file | `~/.config/mpss3-LoongArch-port/micsmc-qt.conf` (QSettings) |
| UI language | key `lang` (`zh` / `en`); falls back to the system locale when unset |
| Temperature thresholds | keys `MicSettingsCpuStat/coreTempLimit` (default 85), `coreTempCaution` (default +10), `coreTempCritical` (default +20) |
| Theme | `MICSMC_QT_THEME=dark` (light by default) |

Thresholds for the three alert levels, all configurable:

| Key | Level | Default |
|---|---|---|
| `MicSettingsCpuStat/coreTempLimit` | first (yellow) | 85 °C |
| `MicSettingsCpuStat/coreTempCaution` | second (orange) | first + 10 °C |
| `MicSettingsCpuStat/coreTempCritical` | third (red) | first + 20 °C |

The first threshold keeps the key name and default value (85 °C) used by the MPSS
control panel, so existing configurations stay meaningful. The two higher levels
default to a fixed offset because libmicmgmt exposes no API for reading temperature
thresholds; adjust them to suit your card.

For example, to raise the first threshold to 90 °C (a card that normally runs hot):

```bash
# quit the application, then edit ~/.config/mpss3-LoongArch-port/micsmc-qt.conf
[MicSettingsCpuStat]
coreTempLimit=90
```

---

## 7. Project layout

| File | Responsibility |
|---|---|
| `src/Snapshot.h` | Cross-thread snapshot struct (filled by the sampler, delivered over a queued signal) |
| `src/DeviceWorker.{h,cpp}` | Worker thread: enumeration / one-second polling / control; every miclib call lives here |
| `src/PlotWidget.{h,cpp}` | Dual-Y-axis time-series chart (60-second window, optional fixed Y range) |
| `src/CoreGridWidget.{h,cpp}` | Per-core utilisation histogram (bar count follows the device's core count; hover tooltip) |
| `src/CardView.{h,cpp}` | Overview panel, per-card panel, right-hand field widgets, legend |
| `src/MainWindow.{h,cpp}` | Main window, menu bar, rendering of poll results |
| `src/SettingsDialog.{h,cpp}` | Advanced Settings dialog |
| `src/I18n.h` | Header-only bilingual string table (key → {zh, en}) |
| `res/original.qss` | Default light theme (embedded via qrc) |
| `res/dark.qss` | Dark theme (embedded via qrc) |
| `res/micsmc-qt.desktop` | Desktop entry |
| `cmake_uninstall.cmake.in` | `make uninstall` script |

---

## 8. Known limitations

1. **ECC toggling is not implemented** — it needs the full maintenance-mode sequence;
   the checkbox is greyed out and labelled accordingly.
2. **Historical Utilization View is not exposed in the UI** — the view enum exists,
   but the panel currently offers only the utilisation and core-histogram buttons.
3. `Error Log`, `Update Application` and `Info` under `Advanced ▾` are placeholders
   (they correspond to error-log analysis, update checking and a card information
   dialog).
4. The second and third temperature thresholds are offset-based defaults (see §6);
   tune them to your card.
5. The project is at an early stage — configuration keys and UI details may still
   change.

---

## 9. License and third-party notices

This project's source code is released under the **MIT License**; see
[LICENSE](LICENSE).

Third parties and trademarks:

- Intel, Xeon Phi and MPSS are trademarks or registered trademarks of Intel
  Corporation. This is an independent third-party implementation and is neither
  affiliated with nor endorsed by Intel.
- At run time it depends on Intel MPSS's `libmicmgmt` and `libscif`, each under its
  own license. No code from those libraries is included here.
- The UI is built on Qt 6 (LGPLv3 / commercial dual license). Qt is linked
  dynamically and its sources are unmodified.
- The colour scheme and layout follow the look of the MPSS control panel; only
  stylistic elements such as colours and dimensions are involved.
