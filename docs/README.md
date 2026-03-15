<div align="center">

<img src="https://i2.wp.com/blitterstudio.com/wp-content/uploads/2020/01/Logo-v3-1.png?resize=768%2C543&ssl=1" alt="Amiberry Logo" width="400"/>

<h1>Amiberry</h1>
<strong>Optimized Amiga Emulator for Linux, macOS, Windows and Android</strong>

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

**Amiberry** is an optimized Amiga emulator designed for ARM (32/64-bit), x86_64, and RISC-V platforms. It natively runs on **Linux**, **macOS**, **Windows** and **Android**, providing a high-performance emulation experience suitable for everything from low-power SBCs (like Raspberry Pi) to powerful desktop workstations and mobile devices.

Built on the core of [WinUAE](https://www.winuae.net), Amiberry brings industry-standard compatibility to multiple platforms while adding unique features tailored for modern setups.

## ✨ Features

-   **High Performance JIT**: Custom Just-In-Time compiler for extreme speed on ARM64 (macOS, Linux, Android) and x86-64 (Linux, macOS, Windows).
-   **WHDLoad BOOTER**: Native support for launching WHDLoad titles directly, handling all configuration automatically.
-   **RetroArch Integration**: Seamless mapping for RetroArch controllers.
-   **Host Tools Integration**: Launch host applications directly from the emulation.
-   **Dynamic File Handling**: Drag-and-drop support for floppies, hardfiles, and config files.
-   **Custom Bezel Overlays**: Use your own bezel PNG images (e.g., CRT monitor frames) with any shader. The emulator output is automatically fitted to the bezel's transparent screen area.
-   **Native GUI**: A responsive, feature-rich interface designed for both mouse and controller navigation.
-   **Auto-Update**: Built-in update checker that detects new releases and downloads them directly — with SHA256 verification.

## 🚀 Installation

### Linux 🐧

**Via package repository (recommended — automatic updates):**
```bash
curl -fsSL https://packages.amiberry.com/install.sh | sudo sh
sudo apt install amiberry      # Debian / Ubuntu / Raspberry Pi OS
sudo dnf install amiberry      # Fedora
```

The repository at [packages.amiberry.com](https://packages.amiberry.com) supports Ubuntu 22.04/24.04/25.10, Debian 12/13, and Fedora. Once set up, `apt upgrade` or `dnf update` will deliver future releases automatically.

**Via Ubuntu PPA:**
```bash
sudo add-apt-repository ppa:midwan-a/amiberry
sudo apt update && sudo apt install amiberry
```

**Via Fedora COPR:**
```bash
sudo dnf copr enable midwan/amiberry
sudo dnf install amiberry
```

**Via Flatpak (Flathub):**
```bash
flatpak install flathub com.blitterstudio.amiberry
```

**Via Arch Linux (AUR):**
```bash
yay -S amiberry
```

**Manual download:**
Download the latest `.deb` or `.rpm` from [GitHub Releases](https://github.com/BlitterStudio/amiberry/releases/latest) and install with `sudo apt install ./amiberry_*.deb` or `sudo dnf install ./amiberry-*.rpm`.

### macOS 🍎

Amiberry supports both Intel (x86_64) and Apple Silicon (M1/M2/M3).

**Via Homebrew:**
```bash
brew install --cask amiberry
```

**Via DMG:**
1.  Download the latest `.dmg` from [Releases](https://github.com/BlitterStudio/amiberry/releases/latest).
2.  Open the disk image.
3.  Drag `Amiberry.app` to your `Applications` folder.

> **Note**: For development builds, file associations for `.uae`, `.adf`, and `.lha` are set up automatically.

### Android 🤖

Amiberry supports Android on AArch64 and x86_64, with full ARM64 JIT support for maximum performance.

See [Compile from Source](https://github.com/BlitterStudio/amiberry/wiki/Compile-from-source) for build instructions.

### Windows 🪟

Amiberry supports Windows x86_64 using MinGW-w64 (GCC).

**Releases:**
1.  Download the latest `.zip` from [Releases](https://github.com/BlitterStudio/amiberry/releases/latest).
2.  Extract the contents to a directory of your choice.
3.  Run `Amiberry.exe` to start the emulator.

## 📚 Documentation

For detailed configuration guides, tutorials, and compatibility lists, please visit the **[Official Wiki](https://github.com/BlitterStudio/amiberry/wiki)**.

-   [First Installation Guide](https://github.com/BlitterStudio/amiberry/wiki/First-Installation)
-   [Compile from Source](https://github.com/BlitterStudio/amiberry/wiki/Compile-from-source)
-   [Troubleshooting](https://github.com/BlitterStudio/amiberry/wiki/Troubleshooting)

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
