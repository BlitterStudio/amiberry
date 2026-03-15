# Installing Amiberry on Linux via Package Repository

Amiberry provides a self-hosted APT/YUM package repository at `packages.amiberry.com`,
allowing you to install and receive automatic updates through your system's package manager.

> **Alternative**: Amiberry is also available on [Flathub](https://flathub.org/apps/com.blitterstudio.amiberry),
> which works on all major Linux distributions without any additional configuration.

---

## Quick Install (One-Liner)

```bash
curl -fsSL https://packages.amiberry.com/install.sh | sudo sh
```

This script auto-detects your distribution and configures the repository.
After setup, install with `sudo apt install amiberry` (Debian/Ubuntu) or `sudo dnf install amiberry` (Fedora).

---

## Manual Installation — Ubuntu / Debian

### Modern format (Ubuntu 24.04+, Debian 12+)

```bash
# Download GPG key
curl -fsSL https://packages.amiberry.com/gpg.key \
  | sudo tee /usr/share/keyrings/amiberry-archive-keyring.gpg > /dev/null

# Add repository (DEB822 format)
echo "Types: deb
URIs: https://packages.amiberry.com/apt
Suites: $(. /etc/os-release && echo $VERSION_CODENAME)
Components: main
Architectures: $(dpkg --print-architecture)
Signed-By: /usr/share/keyrings/amiberry-archive-keyring.gpg" \
  | sudo tee /etc/apt/sources.list.d/amiberry.sources

# Install
sudo apt update && sudo apt install amiberry
```

### Legacy format (Ubuntu 22.04, or if DEB822 format is not supported)

```bash
# Download GPG key
curl -fsSL https://packages.amiberry.com/gpg.key \
  | sudo tee /usr/share/keyrings/amiberry-archive-keyring.gpg > /dev/null

# Add repository (legacy one-line format)
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/amiberry-archive-keyring.gpg] \
  https://packages.amiberry.com/apt $(. /etc/os-release && echo $VERSION_CODENAME) main" \
  | sudo tee /etc/apt/sources.list.d/amiberry.list

# Install
sudo apt update && sudo apt install amiberry
```

### Update Amiberry (APT)

```bash
sudo apt update && sudo apt upgrade amiberry
```

---

## Manual Installation — Fedora

```bash
# Add repository
sudo curl -fsSL https://packages.amiberry.com/rpm/amiberry.repo \
  -o /etc/yum.repos.d/amiberry.repo

# Import GPG key
sudo rpm --import https://packages.amiberry.com/gpg.asc

# Install
sudo dnf install amiberry
```

### Update Amiberry (DNF)

```bash
sudo dnf update amiberry
```

---

## Supported Distributions

| Distribution | Version | Architectures |
|-------------|---------|---------------|
| Ubuntu | 22.04 LTS (Jammy Jellyfish) | amd64 |
| Ubuntu | 24.04 LTS (Noble Numbat) | amd64 |
| Ubuntu | 25.10 (Plucky Puffin) | amd64, arm64 |
| Debian | 12 (Bookworm) | amd64, arm64 |
| Debian | 13 (Trixie) | amd64, arm64 |
| Fedora | Latest | x86_64, aarch64 |

> **Note**: Raspberry Pi OS (64-bit) users can use the Debian Bookworm (arm64) package.

---

## GPG Key Information

The repository is signed with the following key:

| Property | Value |
|----------|-------|
| **Fingerprint** | `D74D52525340C442A4F8B657A12B57C04E1FE282` |
| **Identity** | Amiberry Package Signing <packages@amiberry.com> |
| **Algorithm** | RSA 4096 |
| **Key URL** | https://packages.amiberry.com/gpg.key |

To verify the key:
```bash
curl -fsSL https://packages.amiberry.com/gpg.key | gpg --show-keys
```

---

## Troubleshooting

### "GPG error: NO\_PUBKEY ..."

The GPG key is missing or outdated. Re-run the key import step:

```bash
curl -fsSL https://packages.amiberry.com/gpg.key \
  | sudo tee /usr/share/keyrings/amiberry-archive-keyring.gpg > /dev/null
```

### "Release file for ... is not valid yet / has expired"

This can happen if Amiberry has not published a release in a while. Temporary workaround:

```bash
sudo apt update --allow-releaseinfo-change
```

If this recurs frequently, [open an issue](https://github.com/BlitterStudio/amiberry/issues).

### "Package 'amiberry' has no installation candidate"

Possible causes:
- Your distribution version is not in the supported list above
- Your architecture is not supported for your distro version (e.g., `arm64` on Ubuntu 22.04)
- The repository was not configured correctly — re-run the install steps

### Conflicting versions (Flatpak + APT/DNF both installed)

If you have both the Flatpak and native package installed, the native APT/DNF version
takes precedence in the application launcher. To use only one:

```bash
# Remove Flatpak version (keep native)
flatpak uninstall com.blitterstudio.amiberry

# OR remove native package (keep Flatpak)
sudo apt remove amiberry  # or: sudo dnf remove amiberry
```

---

## Links

- [Amiberry Homepage](https://amiberry.com)
- [GitHub Repository](https://github.com/BlitterStudio/amiberry)
- [Flathub (Alternative Install)](https://flathub.org/apps/com.blitterstudio.amiberry)
- [Issue Tracker](https://github.com/BlitterStudio/amiberry/issues)
