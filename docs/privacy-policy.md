---
layout: default
title: Privacy Policy - Amiberry
---

# Privacy Policy

**Effective Date: March 28, 2026**

This privacy policy describes how **Amiberry** ("the App"), developed by **BlitterStudio**, handles user data on the Android platform.

---

## Summary

Amiberry does not collect, store, or transmit any personal data, with the sole exception of **anonymous crash reports** sent via Firebase Crashlytics to help us fix bugs. No personal information, usage patterns, or file contents are ever collected.

---

## What Data Does the App Access?

Amiberry accesses files stored locally on your device, including:

- Amiga ROM files (Kickstart ROMs)
- Disk image files (ADF, IPF, HDF, WHDLoad archives)
- Configuration files created by the App
- WHDLoad game data

All file access is local to your device. No files are uploaded, transmitted, or shared with any third party.

## Why Does the App Need Storage Permission?

Amiberry requests the `MANAGE_EXTERNAL_STORAGE` (All Files Access) permission on Android 11 and above. This permission is required because:

- Users store their Amiga ROM files and disk images in various locations on their device
- The App must be able to read these files from any user-chosen directory to function as an emulator
- Without this permission, the App cannot access files outside its own private storage area

## Network Usage

Amiberry uses the `INTERNET` permission for:

1. **Emulated Amiga networking** (bsdsocket.library emulation) — allows Amiga software running inside the emulator to access network resources, just as a real Amiga would.
2. **Crash reporting** — anonymous crash diagnostics are sent to Firebase Crashlytics (see below).

The App does not:

- Send analytics or telemetry data
- Display advertisements
- Track user behavior or usage patterns
- Make any other network requests on its own behalf

## Crash Reporting (Firebase Crashlytics)

Amiberry uses **Firebase Crashlytics** to collect anonymous crash reports. This helps us identify and fix bugs, which is especially important for a complex emulator with native code.

**What Crashlytics collects when a crash occurs:**

- Crash stack trace (technical details about what went wrong)
- Device model and manufacturer
- Android OS version
- App version and build number
- A random Crashlytics installation ID (not linked to your identity)
- General device state (e.g. available memory, orientation)

**What Crashlytics does NOT collect:**

- Your name, email, or any personal identifiers
- File names or contents on your device
- ROM files, disk images, or any emulation data
- Your location
- Any information about what you were emulating

Crashlytics data is processed by Google in accordance with the [Firebase Terms of Service](https://firebase.google.com/terms) and the [Google Privacy Policy](https://policies.google.com/privacy). Crash data is retained for 90 days.

## Other Data Collection

Beyond crash reports, **Amiberry does not collect any data.** Specifically:

- No personal information is collected
- No usage statistics or analytics are gathered
- No advertising identifiers are recorded
- No cookies or tracking technologies are used

## Third-Party Services

Amiberry integrates **Firebase Crashlytics** for crash reporting as described above. There are no ads, analytics, social media integrations, or other third-party services that collect user data.

## Children's Privacy

Amiberry does not knowingly collect personal information from children. The only data collected (crash reports) is anonymous and not linked to any individual. The App complies with applicable children's privacy regulations.

## Data Security

No personal data is collected or transmitted. Crash reports contain only anonymous technical information and are transmitted securely (HTTPS) to Firebase servers. All emulator data (configurations, save states) is stored locally on your device and is under your control.

## Open Source

Amiberry is open-source software released under the **GNU General Public License v3 (GPLv3)**. The complete source code is publicly available at [github.com/BlitterStudio/amiberry](https://github.com/BlitterStudio/amiberry), allowing full transparency into how the App operates.

## Changes to This Policy

If this privacy policy is updated, the revised version will be posted at this URL with an updated effective date.

## Contact

If you have questions about this privacy policy, you can reach us at:

- **GitHub:** [github.com/BlitterStudio/amiberry](https://github.com/BlitterStudio/amiberry)
- **Discord:** [discord.gg/wWndKTGpGV](https://discord.gg/wWndKTGpGV)

---

[Back to Amiberry home](/)
