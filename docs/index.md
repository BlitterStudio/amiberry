<nav class="nav-bar">
  <a href="#features">Features</a>
  <a href="#get-amiberry">Install</a>
  <a href="#documentation">Docs</a>
  <a href="#community">Community</a>
</nav>

<div class="hero">
  <img src="https://i2.wp.com/blitterstudio.com/wp-content/uploads/2020/01/Logo-v3-1.png?resize=768%2C543&ssl=1" alt="Amiberry Logo"/>

  <div class="hero-badges">
    <a href="https://github.com/BlitterStudio/amiberry/actions/workflows/c-cpp.yml">
      <img src="https://github.com/BlitterStudio/amiberry/actions/workflows/c-cpp.yml/badge.svg" alt="C/C++ CI"/>
    </a>
    <a href="https://nightly.link/BlitterStudio/amiberry/workflows/c-cpp.yml/master">
      <img src="https://img.shields.io/badge/Dev%20Builds-nightly.link-orange" alt="Development Builds"/>
    </a>
    <a href="https://discord.gg/wWndKTGpGV">
      <img src="https://img.shields.io/badge/Discord-Chat-5865F2?logo=discord&logoColor=white" alt="Discord"/>
    </a>
    <a href="https://ko-fi.com/X8X4FHDY4">
      <img src="https://img.shields.io/badge/Ko--fi-Support-FF5E5B?logo=ko-fi&logoColor=white" alt="Ko-fi"/>
    </a>
  </div>

  <p class="hero-description">
    Built on the WinUAE core, Amiberry delivers full Amiga compatibility across ARM and x86 hardware.
    From a Raspberry Pi to a desktop workstation, from Linux to Android, FreeBSD to Haiku — the Amiga runs everywhere.
  </p>

  <div class="hero-cta">
    <a href="https://github.com/BlitterStudio/amiberry/releases/latest" class="btn-primary">Get Latest Release</a>
    <a href="https://github.com/BlitterStudio/amiberry/wiki" class="btn-secondary">Read the Wiki</a>
  </div>
</div>

<div class="highlight-grid">
  <div class="highlight-card">
    <div class="highlight-icon">⚡</div>
    <h3>Blazing Fast</h3>
    <p>Custom JIT compiler for ARM64 and x86-64 delivers maximum emulation speed</p>
  </div>
  <div class="highlight-card">
    <div class="highlight-icon">🎯</div>
    <h3>Highly Compatible</h3>
    <p>Industry-standard WinUAE emulation core with proven accuracy</p>
  </div>
  <div class="highlight-card">
    <div class="highlight-icon">🌍</div>
    <h3>Truly Multi-Platform</h3>
    <p>Linux, macOS, Windows, Android, FreeBSD, and Haiku on ARM, x86, and RISC-V</p>
  </div>
</div>

---

## Features

Everything you need for the ultimate Amiga experience.

<div class="feature-grid">
  <div class="feature-card">
    <div class="feature-icon">⚡</div>
    <div class="feature-content">
      <h4>JIT Compiler</h4>
      <p>Custom just-in-time compilation for ARM64 and x86-64 — maximum speed on modern hardware</p>
    </div>
  </div>
  <div class="feature-card">
    <div class="feature-icon">🎮</div>
    <div class="feature-content">
      <h4>WHDLoad Support</h4>
      <p>Launch WHDLoad titles directly with automatic configuration — no manual setup required</p>
    </div>
  </div>
  <div class="feature-card">
    <div class="feature-icon">🖼️</div>
    <div class="feature-content">
      <h4>Custom Bezels</h4>
      <p>Apply CRT monitor frames and overlay effects with full shader support</p>
    </div>
  </div>
  <div class="feature-card">
    <div class="feature-icon">🎛️</div>
    <div class="feature-content">
      <h4>RetroArch Ready</h4>
      <p>Seamless controller mapping for RetroArch setups out of the box</p>
    </div>
  </div>
  <div class="feature-card">
    <div class="feature-icon">🖥️</div>
    <div class="feature-content">
      <h4>Modern GUI</h4>
      <p>Clean, responsive interface navigable by mouse or gamepad</p>
    </div>
  </div>
  <div class="feature-card">
    <div class="feature-icon">📂</div>
    <div class="feature-content">
      <h4>Drag &amp; Drop</h4>
      <p>Drop floppy images, hard files, and config files directly into the emulator</p>
    </div>
  </div>
  <div class="feature-card">
    <div class="feature-icon">🔄</div>
    <div class="feature-content">
      <h4>Auto-Update</h4>
      <p>Built-in update checker with SHA256-verified downloads</p>
    </div>
  </div>
  <div class="feature-card">
    <div class="feature-icon">✨</div>
    <div class="feature-content">
      <h4>Shader Support</h4>
      <p>Built-in shaders plus support for external GLSL shaders for the perfect retro look</p>
    </div>
  </div>
</div>

---

## Get Amiberry

Choose your platform and start emulating in minutes.

<div class="platform-grid">
  <div class="platform-card linux">
    <h3>🐧 Linux</h3>
    <p>Install via the official package repository:</p>
    <pre><code>curl -fsSL https://packages.amiberry.com/install.sh | sudo sh
sudo apt install amiberry</code></pre>
    <p class="platform-alt">
      Also available via
      <a href="https://launchpad.net/~midwan-a/+archive/ubuntu/amiberry">PPA</a> ·
      <a href="https://copr.fedorainfracloud.org/coprs/midwan/amiberry/">COPR</a> ·
      <a href="https://flathub.org/apps/com.blitterstudio.amiberry">Flatpak</a> ·
      <a href="https://aur.archlinux.org/packages/amiberry">AUR</a> ·
      <a href="https://github.com/BlitterStudio/amiberry/releases/latest">.deb/.rpm</a>
    </p>
  </div>

  <div class="platform-card macos">
    <h3>🍎 macOS</h3>
    <p>Install via Homebrew (Intel &amp; Apple Silicon):</p>
    <pre><code>brew install --cask amiberry</code></pre>
    <p class="platform-alt">
      Also available as a
      <a href="https://github.com/BlitterStudio/amiberry/releases/latest">.dmg download</a>
    </p>
  </div>

  <div class="platform-card windows">
    <h3>🪟 Windows</h3>
    <p>Download from <a href="https://github.com/BlitterStudio/amiberry/releases/latest">Releases</a>:</p>
    <ul>
      <li><strong>Installer</strong> — run the <code>.exe</code> installer for a guided setup</li>
      <li><strong>Portable</strong> — extract the <code>.zip</code> to any directory and run <code>Amiberry.exe</code></li>
    </ul>
  </div>

  <div class="platform-card android">
    <h3>🤖 Android</h3>
    <p>AArch64 &amp; x86_64 with full ARM64 JIT support for maximum performance.</p>
    <p class="platform-alt">
      See the <a href="https://github.com/BlitterStudio/amiberry/wiki/Compile-from-source">build instructions</a> for details.
    </p>
  </div>
</div>

---

## Documentation

Guides, tutorials, and references to help you get the most out of Amiberry.

<div class="resource-grid">
  <a href="https://github.com/BlitterStudio/amiberry/wiki/First-Installation" class="resource-link">
    <span class="resource-icon">🚀</span>
    Getting Started
  </a>
  <a href="https://github.com/BlitterStudio/amiberry/wiki" class="resource-link">
    <span class="resource-icon">📘</span>
    Full Wiki
  </a>
  <a href="https://github.com/BlitterStudio/amiberry/wiki/Compile-from-source" class="resource-link">
    <span class="resource-icon">🔨</span>
    Build from Source
  </a>
  <a href="https://github.com/BlitterStudio/amiberry/wiki/Troubleshooting" class="resource-link">
    <span class="resource-icon">🔍</span>
    Troubleshooting
  </a>
</div>

---

## Community

Join the conversation, get help, and contribute to the project.

<div class="community-links">
  <a href="https://discord.gg/wWndKTGpGV">
    <img src="https://img.shields.io/badge/Discord-Join%20Chat-5865F2?style=for-the-badge&logo=discord&logoColor=white" alt="Discord"/>
  </a>
  <a href="https://mastodon.social/@midwan">
    <img src="https://img.shields.io/badge/Mastodon-Follow-6364FF?style=for-the-badge&logo=mastodon&logoColor=white" alt="Mastodon"/>
  </a>
  <a href="https://ko-fi.com/X8X4FHDY4">
    <img src="https://img.shields.io/badge/Ko--fi-Support-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white" alt="Ko-fi"/>
  </a>
</div>

### Contributing

Contributions are welcome — bug reports, feature suggestions, and pull requests all help make Amiberry better.

1. Fork the repository
2. Create your feature branch: `git checkout -b feature/my-feature`
3. Commit your changes: `git commit -m 'Add my feature'`
4. Push to the branch: `git push origin feature/my-feature`
5. Open a Pull Request

---

<div class="site-footer">
  <div class="sponsor-section">
    <p class="sponsor-label">Supported by</p>
    <a href="https://jb.gg/OpenSourceSupport">
      <img src="https://resources.jetbrains.com/storage/products/company/brand/logos/jetbrains.svg" alt="JetBrains" width="100"/>
    </a>
  </div>
  <p class="footer-meta">
    Licensed under <a href="https://github.com/BlitterStudio/amiberry/blob/master/LICENSE">GPLv3</a>
  </p>
</div>
