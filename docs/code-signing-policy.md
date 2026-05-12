# Code Signing Policy

Windows release builds published by Amiberry are signed through SignPath. Free code signing provided by [SignPath.io](https://about.signpath.io/), certificate by [SignPath Foundation](https://signpath.org/).

## Scope

SignPath signing is used for Amiberry-owned Windows release binaries built by GitHub Actions from the [BlitterStudio/amiberry](https://github.com/BlitterStudio/amiberry) source repository.

The signed Windows artifacts are:

- the Windows installer executable
- `Amiberry.exe` inside the portable ZIP package

Bundled third-party DLLs are not signed with the Amiberry SignPath subscription. They are included as open-source runtime dependencies produced by the CI build.

Release signing requests are submitted from GitHub-hosted Actions runners and require manual approval in SignPath before release-signed artifacts are published.

## Roles

- Committers and reviewers: `@midwan` and repository collaborators with write access to [BlitterStudio/amiberry](https://github.com/BlitterStudio/amiberry).
- Approvers: `@midwan`, using SignPath approval permissions for Amiberry release signing.

## Privacy

Amiberry does not transfer emulator configuration, ROMs, disk images, save data, or personal files to the Amiberry project.

Desktop builds can contact GitHub when the update-check feature is enabled or when the user starts an update/download action. Emulated networking only sends traffic when the user configures and runs networking inside the emulated Amiga environment.

The Android app privacy policy is published separately at [amiberry.com/privacy-policy](https://amiberry.com/privacy-policy).
