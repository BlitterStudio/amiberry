# One-Time Fedora COPR Setup

This document describes the **one-time manual steps** to set up the Fedora COPR project.
After setup, RPM builds are automated by Packit when a GitHub Release is published.

## Prerequisites

- A Fedora Account System (FAS) account at https://accounts.fedoraproject.org
- The Packit GitHub App installed on the BlitterStudio/amiberry repository

## Steps

### 1. Create Fedora Account
Go to https://accounts.fedoraproject.org and create an account.

### 2. Create COPR Project
Go to https://copr.fedorainfracloud.org/coprs/blitterstudio/
Click "New Project":
- Name: `amiberry`
- Description: `Optimized Amiga emulator for Fedora`
- Chroots: `fedora-latest-x86_64`, `fedora-latest-aarch64`

### 3. Install Packit GitHub App
Go to https://github.com/marketplace/packit-as-a-service
Install for the `BlitterStudio/amiberry` repository.

### 4. Configure COPR to Accept Packit
In COPR project settings → Packages → Allow builds from Packit.

### 5. Verify

After the next GitHub Release is published, Packit will:
1. Create a source RPM from the spec file
2. Submit a build to COPR
3. Make packages available at: https://copr.fedorainfracloud.org/coprs/blitterstudio/amiberry

### COPR Repository URL for users

```bash
sudo dnf install dnf-plugins-core
sudo dnf copr enable blitterstudio/amiberry
sudo dnf install amiberry
```
