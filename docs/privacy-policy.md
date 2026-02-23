---
layout: default
title: Privacy Policy - Amiberry
---

# Privacy Policy

**Effective Date: February 23, 2026**

This privacy policy describes how **Amiberry** ("the App"), developed by **BlitterStudio**, handles user data on the Android platform.

---

## Summary

Amiberry does not collect, store, or transmit any personal data. The App runs entirely on your device and does not communicate with any external servers.

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

Amiberry uses the `INTERNET` permission solely for **emulated Amiga networking** (bsdsocket.library emulation). This allows Amiga software running inside the emulator to access network resources, just as a real Amiga would.

The App itself does not:

- Contact any external servers
- Send analytics or telemetry data
- Perform crash reporting
- Display advertisements
- Make any network requests on its own behalf

## Data Collection

**Amiberry does not collect any data.** Specifically:

- No personal information is collected
- No usage statistics or analytics are gathered
- No device identifiers are recorded
- No crash reports are sent
- No cookies or tracking technologies are used

## Third-Party Services

Amiberry does not integrate any third-party services, SDKs, or libraries that collect user data. There are no ads, analytics, or social media integrations.

## Children's Privacy

Since Amiberry does not collect any data from any user, it does not collect data from children. The App complies with applicable children's privacy regulations.

## Data Security

Because no personal data is collected or transmitted, there is no user data at risk. All emulator data (configurations, save states) is stored locally on your device and is under your control.

## Open Source

Amiberry is open-source software released under the **GNU General Public License v3 (GPLv3)**. The complete source code is publicly available at [github.com/BlitterStudio/amiberry](https://github.com/BlitterStudio/amiberry), allowing full transparency into how the App operates.

## Changes to This Policy

If this privacy policy is updated, the revised version will be posted at this URL with an updated effective date. Since the App does not collect data, changes are unlikely.

## Contact

If you have questions about this privacy policy, you can reach us at:

- **GitHub:** [github.com/BlitterStudio/amiberry](https://github.com/BlitterStudio/amiberry)
- **Discord:** [discord.gg/wWndKTGpGV](https://discord.gg/wWndKTGpGV)

---

[Back to Amiberry home](/)
