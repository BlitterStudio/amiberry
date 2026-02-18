<div align="center">

<img src="https://i2.wp.com/blitterstudio.com/wp-content/uploads/2020/01/Logo-v3-1.png?resize=768%2C543&ssl=1" alt="Amiberry Logo" width="400"/>

<h1>Amiberry</h1>
<strong>Optimized Amiga Emulator for Linux, macOS and Windows</strong>

<p>
  <a href="https://github.com/BlitterStudio/amiberry/actions/workflows/c-cpp.yml">
    <img src="https://github.com/BlitterStudio/amiberry/actions/workflows/c-cpp.yml/badge.svg" alt="C/C++ CI"/>
  </a>
  <a href="https://nightly.link/BlitterStudio/amiberry/workflows/c-cpp.yml/master">
    <img src="https://img.shields.io/badge/Development%20Builds-nightly.link-orange" alt="Development Builds"/>
  </a>
  <a href="https://discord.gg/wWndKTGpGV">
    <img src="https://img.shields.io/badge/Discord-Join%20Chat-5865F2?logo=discord&logoColor=white" alt="Discord"/>
  </a>
  <a href="https://mastodon.social/@midwan">
    <img src="https://img.shields.io/badge/Mastodon-Follow-6364FF?logo=mastodon&logoColor=white" alt="Mastodon"/>
  </a>
  <a href="https://ko-fi.com/X8X4FHDY4">
    <img src="https://img.shields.io/badge/Ko--fi-Support-FF5E5B?logo=ko-fi&logoColor=white" alt="Ko-Fi"/>
  </a>
</p>

</div>

---

## 📖 Introduction

**Amiberry** is an optimized Amiga emulator designed for ARM (32/64-bit), x86_64, and RISC-V platforms. It natively runs on **Linux**, **macOS** and **Windows**, providing a high-performance emulation experience suitable for everything from low-power SBCs (like Raspberry Pi) to powerful desktop workstations.

Built on the core of [WinUAE](https://www.winuae.net), Amiberry brings industry-standard compatibility to multiple platforms while adding unique features tailored for modern setups.

## ✨ Features

-   **High Performance JIT**: Custom Just-In-Time compiler for extreme speed on supported devices.
-   **WHDLoad BOOTER**: Native support for launching WHDLoad titles directly, handling all configuration automatically.
-   **RetroArch Integration**: Seamless mapping for RetroArch controllers.
-   **Host Tools Integration**: Launch host applications directly from the emulation.
-   **Dynamic File Handling**: Drag-and-drop support for floppies, hardfiles, and config files.
-   **Native GUI**: A responsive, feature-rich interface designed for both mouse and controller navigation.

## 🚀 Installation

### Linux 🐧

Amiberry is available as a `.deb` (Debian/Ubuntu/Raspberry Pi OS) and `.rpm` (Fedora/RHEL) package.

**Debian / Ubuntu / Raspberry Pi OS:**
```bash
# Download the latest .deb from Releases or Development Builds
sudo apt update
sudo apt install ./amiberry_*.deb
```

**Fedora:**
```bash
# Download the latest .rpm from Releases or Development Builds
sudo dnf install ./amiberry-*.rpm
```

**Flatpak (Flathub):**
```bash
flatpak install flathub com.blitterstudio.amiberry
```

**Arch Linux (AUR):**
```bash
yay -S amiberry
```

### macOS 🍎

Amiberry supports both Intel (x86_64) and Apple Silicon (M1/M2/M3).

**Via Homebrew:**
```bash
brew install --cask amiberry
```

**Via DMG:**
1.  Download the latest `.dmg` from [Releases](https://github.com/BlitterStudio/amiberry/releases).
2.  Open the disk image.
3.  Drag `Amiberry.app` to your `Applications` folder.

> **Note**: For development builds, file associations for `.uae`, `.adf`, and `.lha` are set up automatically.

### Windows 🪟

Amiberry supports Windows x86_64 using MinGW-w64 (GCC).

**Releases:**
1.  Download the latest `.zip` from [Releases](https://github.com/BlitterStudio/amiberry/releases).
2.  Extract the contents to a directory of your choice.
3.  Run `Amiberry.exe` to start the emulator.

## 📚 Documentation

For detailed configuration guides, tutorials, and compatibility lists, please visit the **[Official Wiki](https://github.com/BlitterStudio/amiberry/wiki)**.

-   [First Installation Guide](https://github.com/BlitterStudio/amiberry/wiki/First-Installation)
-   [Compile from Source](https://github.com/BlitterStudio/amiberry/wiki/Compile-from-source)
-   [Troubleshooting](https://github.com/BlitterStudio/amiberry/wiki/Troubleshooting)

### ARM64 JIT diagnostics

On ARM64 hosts, JIT includes runtime safety guards for known unstable startup paths.

- `AMIBERRY_ARM64_GUARD_VERBOSE=1` enables detailed guard-learning logs (diagnostic use).
- `AMIBERRY_ARM64_DISABLE_HOTSPOT_GUARD=1` disables optional hotspot logic only; fixed safety fallback for the known startup hotspot remains active.

## 🤝 Contributing

Contributions are welcome! Whether it's reporting bugs, suggesting features, or submitting Pull Requests, your help makes Amiberry better.

1.  Fork the repository.
2.  Create your feature branch (`git checkout -b feature/AmazingFeature`).
3.  Commit your changes (`git commit -m 'Add some AmazingFeature'`).
4.  Push to the branch (`git push origin feature/AmazingFeature`).
5.  Open a Pull Request.

## 📄 License

This project is licensed under the **GNU General Public License v3.0 (GPLv3)**.  
See the [LICENSE](LICENSE) file for details.

---

<div align="center">

<p><sub>Supported by</sub></p>
<a href="https://jb.gg/OpenSourceSupport">
  <img src="https://resources.jetbrains.com/storage/products/company/brand/logos/jetbrains.svg" alt="JetBrains" width="100"/>
</a>

</div>
