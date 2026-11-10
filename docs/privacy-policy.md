---
layout: default
title: Privacy Policy - Amiberry
---

# Privacy Policy

**Effective Date: November 9, 2026**

This privacy policy describes how **Amiberry** ("the App"), developed by Dimitris Panokostas (BlitterStudio), handles your data in:

- Amiberry for iPhone, iPad and Mac, from Apple's App Store;
- Amiberry for Android, from Google Play;
- the builds you download from GitHub or amiberry.com, and the packages built from them.

---

## Summary

- **iPhone, iPad and Mac (App Store):** the App collects no data at all. App Store "Data Not Collected".
- **Android:** the only data that leaves your device is an **anonymous crash report** sent through Firebase Crashlytics when the App crashes.
- On every platform there are no accounts, no analytics, no advertising and no tracking. Your files never leave your device.

---

## Files the App Uses

Amiberry reads and writes files on your device so it can emulate an Amiga:

- Kickstart ROM files
- Floppy, hard disk and CD images (ADF, IPF, HDF, ISO, CHD and others), and WHDLoad archives
- Configuration files, save states and screenshots it creates
- WHDLoad boot files and the game database

All of this stays on your device. Nothing is uploaded, transmitted or shared.

- **iPhone and iPad:** the App works with its own folder (visible in the Files app) and with files you choose in the Files app or share to Amiberry.
- **Mac (App Store):** the App runs in Apple's sandbox. It can only open files and folders you choose, and it remembers that choice through a sandbox bookmark.
- **Android:** the App asks for "All files access" (`MANAGE_EXTERNAL_STORAGE`) on Android 11 and later, because ROMs and disk images are usually kept in folders you choose, outside the App's private storage.

## Camera and Microphone

On iPhone, iPad and Mac, the App asks for the camera or the microphone only when you select them as an input of the emulated Amiga: the camera as a genlock video source, the microphone as the recording input of the sampler or an emulated sound card. The picture and sound are used live by the emulation. They are not stored or sent anywhere.

## Network Use

The App connects to the internet only in these cases:

1. **Downloads you ask for.** WHDLoad, its helper files and some Amiga libraries are not included with the App, because their licences do not allow it. When you ask for them (for example when you first start a WHDLoad game), the App downloads:
   - WHDLoad from its author's site, `whdload.de`;
   - the kickstart relocation files, the JST booter and Amiga libraries (XPK, MMULib, explode, medplayer) from `aminet.net`;
   - the boot files and the WHDLoad game database from GitHub (`github.com`, `raw.githubusercontent.com`): the Amiberry project's files, or, if its game database can't be reached, the older one from the Amiberry XML Builder project.

   In the versions with the classic settings window (the desktop builds), the "Update Controllers DB" button also downloads the SDL game controller database from GitHub.

   Like any download, these servers see your IP address. The App sends them nothing else.
2. **Emulated networking.** Amiga software running inside the emulator can use the network (bsdsocket.library and the emulated network cards), just as on a real Amiga. That traffic goes where the Amiga software sends it.
3. **Update check (direct downloads and Linux packages, except Flatpak).** These versions can check `api.github.com` for new Amiberry releases. On Windows and macOS you can then download the new release from GitHub. The App Store, Google Play and Flatpak versions don't check: their stores deliver the updates.
4. **Crash reports (Android only).** See below.

Links on the App's About and Licences screens open in your browser.

## Crash Reporting on Android (Firebase Crashlytics)

The Android App uses **Firebase Crashlytics** to send an anonymous report when it crashes. Native emulator code is hard to debug without them.

**What a crash report contains:**

- the crash stack trace;
- device model and manufacturer;
- Android OS version;
- App version and build number;
- a random Crashlytics installation ID, not linked to your identity;
- general device state (for example available memory and orientation).

**What it does not contain:** your name, email or other personal identifiers, file names or file contents, ROMs, disk images or any emulation data, your location, or what you were emulating.

Google processes Crashlytics data under the [Firebase Terms of Service](https://firebase.google.com/terms) and the [Google Privacy Policy](https://policies.google.com/privacy). Crash data is kept for 90 days.

The iPhone, iPad and Mac versions contain no crash reporting.

## Other Data Collection

Beyond the Android crash reports above, **Amiberry collects no data**:

- no personal information;
- no usage statistics or analytics;
- no advertising identifiers;
- no cookies or tracking technologies.

## Third-Party Services

The only third-party service built into Amiberry is Firebase Crashlytics, in the Android version. There are no ads, analytics, social media integrations or other services that collect user data.

## Children's Privacy

Amiberry does not knowingly collect personal information from anyone, children included. The only data it sends (Android crash reports) is anonymous and is not linked to any person.

## Data Security

Downloads and crash reports use HTTPS. Your configurations, save states and other emulator data stay on your device, under your control.

## Open Source

Amiberry is open-source software under the **GNU General Public License v3 (GPLv3)**. The complete source code is at [github.com/BlitterStudio/amiberry](https://github.com/BlitterStudio/amiberry), so anyone can check what the App does. The iPhone, iPad and Mac apps have an About & Licences (Acknowledgements) screen with the GPL text, a link to the exact source of that build, and the licence notices of the third-party components it ships.

## Changes to This Policy

If this policy changes, the new version will be posted at this URL with a new effective date.

## Contact

Questions about this policy:

- **Email:** [midwan@gmail.com](mailto:midwan@gmail.com)
- **GitHub:** [github.com/BlitterStudio/amiberry](https://github.com/BlitterStudio/amiberry)
- **Discord:** [discord.gg/wWndKTGpGV](https://discord.gg/wWndKTGpGV)

---

[Back to Amiberry home](/)
