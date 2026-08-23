<div align="center">
  <img src="packaging/io.github.sappy.SappyControls.svg" width="128" alt="Sappy's Controls logo" />
  <h1>Sappy's Controls</h1>
  <p><strong>A focused Qt 6 control panel for the Linux commands you use every day.</strong></p>

  ![C++](https://img.shields.io/badge/C++20-00599C?style=flat-square&logo=cplusplus&logoColor=white)
  ![Qt](https://img.shields.io/badge/Qt-6-41CD52?style=flat-square&logo=qt&logoColor=white)
  ![Linux](https://img.shields.io/badge/platform-Linux-FCC624?style=flat-square&logo=linux&logoColor=black)
  [![License: GPL v3](https://img.shields.io/badge/license-GPLv3-blue?style=flat-square)](LICENSE)
</div>

Sappy's Controls turns hardware-tuning commands, Wi-Fi management, and network diagnostics into clear buttons, validated inputs, and checkboxes. It was built for an AMD Ryzen laptop using `ryzenadj` and `nbfc-linux`, but keeps the helpers configurable and free of machine-specific credentials.

## Features

- **High Performance profile** — configurable temperature and fixed fan speed with 55 / 75 / 65 W Ryzen limits.
- **Force Idle profile** — configurable temperature with 45 / 45 / 45 W limits and automatic fan control.
- **Custom tuning** — sustained, fast, and slow power limits plus temperature and fixed or automatic fan control.
- **Wi-Fi controls** — unload the configured kernel driver for this boot, disable it persistently, or restore it.
- **Network tools** — launch the bundled Hetzner throughput/bufferbloat test and an optional `speedcheck` TUI.
- **Live activity** — command output, errors, progress state, and a stop control in one place.
- **Desktop integration** — scalable icon and application-menu entry.

## Safety

- Numeric tuning values are range-checked before elevation.
- User input is passed as positional parameters to a fixed privileged sequence; it is never interpolated into free-form shell code.
- Persistent Wi-Fi disable requires confirmation because it changes modprobe/udev configuration and rebuilds initramfs.
- The bundled Wi-Fi helper contains no password handling. It elevates fixed system commands with Polkit in a desktop session or `sudo` in a terminal.
- The repository contains no machine usernames, home-directory paths, private gateway addresses, or credentials.

## Requirements

### Build

- CMake 3.21+
- Ninja
- A C++20 compiler
- Qt 6.5+ with Widgets and Test

### Runtime

- Linux with systemd and Polkit
- `ryzenadj`
- `nbfc-linux`
- `nvidia-powerd` for the original NVIDIA power-service step
- For Wi-Fi control: `kmod`, `rfkill`, udev, and an initramfs tool (`mkinitcpio`, `update-initramfs`, or `dracut`)
- For the network test: Bash, curl, iproute2, iputils, GNU awk/grep, and coreutils
- A supported terminal: Konsole, Kitty, Alacritty, GNOME Terminal, Xfce Terminal, or xterm

`speedcheck` is optional and is detected from `PATH`.

## Install

```bash
git clone https://github.com/ShaptakNaskar/SappyControl.git
cd SappyControl
chmod +x install.sh
./install.sh
```

The default prefix is `~/.local`, so the app appears in your application menu without a system-wide install. To use another prefix:

```bash
SAPPY_CONTROLS_PREFIX=/your/prefix ./install.sh
```

Run it directly with:

```bash
~/.local/bin/sappy-controls
```

## Build manually

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/sappy-controls
```

## Bundled helpers

The installer uses distinct names so it will not overwrite similarly named personal scripts:

- `sappy-wifi-kill`
- `sappy-hetzner-nettest`

The app also recognizes the legacy `wifi-kill` command and `~/hetzner-nettest.sh`.

### Wi-Fi driver configuration

The helper defaults to Intel's `iwlwifi` module and NetworkManager. Override either when launching the app or helper:

```bash
WIFI_DRIVER=your_module NETWORK_MANAGER_SERVICE=your_service sappy-wifi-kill --kill
```

### Hetzner nettest

Run one cycle without writing logs:

```bash
CYCLES=1 sappy-hetzner-nettest --nolog
```

Useful environment knobs include `SIZE`, `CYCLES`, `INTERVAL`, `QUALIFY`, `QUALIFY_SECS`, `WINNER`, `ISP`, `REF`, `RAMDIR`, `LOG`, and `CSV`.

## Project layout

```text
src/          Qt application and safe command builder
scripts/      Sanitized Wi-Fi and network-test helpers
packaging/    Desktop entry and application icons
resources/    Icons embedded into the executable
tests/        Command validation and construction tests
```

## License

Copyright © 2026 Shaptak Naskar.

Sappy's Controls is licensed under the **GNU General Public License v3.0 only**. See [LICENSE](LICENSE) for the complete terms.
