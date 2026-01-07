<div align="center">
  <img src="https://i2.wp.com/blitterstudio.com/wp-content/uploads/2020/01/Logo-v3-1.png?resize=768%2C543&ssl=1" alt="Amiberry Logo" width="400"/>

  # Amiberry
  **Optimized Amiga Emulator for macOS and Linux**

  [![C/C++ CI](https://github.com/BlitterStudio/amiberry/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/BlitterStudio/amiberry/actions/workflows/c-cpp.yml)
  [![Development Builds](https://img.shields.io/badge/Development%20Builds-nightly.link-orange)](https://nightly.link/BlitterStudio/amiberry/workflows/c-cpp.yml/master)
  [![Discord](https://img.shields.io/badge/Discord-Join%20Chat-5865F2?logo=discord&logoColor=white)](https://discord.gg/wWndKTGpGV)
  [![Mastodon](https://img.shields.io/badge/Mastodon-Follow-6364FF?logo=mastodon&logoColor=white)](https://mastodon.social/@midwan)
  [![Ko-Fi](https://img.shields.io/badge/Ko--fi-Support-FF5E5B?logo=ko-fi&logoColor=white)](https://ko-fi.com/X8X4FHDY4)
</div>

---

## 📖 Introduction

**Amiberry** is an optimized Amiga emulator designed for ARM (32/64-bit), x86_64, and RISC-V platforms. It natively runs on **Linux** and **macOS**, providing a high-performance emulation experience suitable for everything from low-power SBCs (like Raspberry Pi) to powerful desktop workstations.

Built on the core of [WinUAE](https://www.winuae.net), Amiberry brings industry-standard compatibility to non-Windows platforms while adding unique features tailored for modern setups.

## ✨ Features

-   **High Performance JIT**: Custom Just-In-Time compiler for extreme speed on ARM devices.
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

## 📚 Documentation

For detailed configuration guides, tutorials, and compatibility lists, please visit the **[Official Wiki](https://github.com/BlitterStudio/amiberry/wiki)**.

-   [First Installation Guide](https://github.com/BlitterStudio/amiberry/wiki/First-Installation)
-   [Compile from Source](https://github.com/BlitterStudio/amiberry/wiki/Compile-from-source)

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
  <sub>Supported by</sub><br/>
  <a href="https://jb.gg/OpenSourceSupport">
    <img src="https://resources.jetbrains.com/storage/products/company/brand/logos/jetbrains.svg" alt="JetBrains" width="100"/>
  </a>
</div>
