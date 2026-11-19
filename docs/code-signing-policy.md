# Code Signing Policy

Windows release builds published by Amiberry are signed with a [Certum](https://www.certum.eu/) Open Source Code Signing certificate issued to the project maintainer.

## Scope

The certificate is used only for Amiberry-owned Windows release binaries, built by GitHub Actions from the [BlitterStudio/amiberry](https://github.com/BlitterStudio/amiberry) source repository.

The signed Windows artifacts are:

- the Windows installer executable
- `Amiberry.exe` inside the portable ZIP package

Bundled third-party DLLs are not signed. They are open-source runtime dependencies produced by the CI build.

Only release tags are signed. The signing key stays in Certum's cloud HSM. Signing runs on GitHub-hosted Actions runners, in a job that can read the signing credentials only for release tags.

## Roles

- Committers and reviewers: `@midwan` and repository collaborators with write access to [BlitterStudio/amiberry](https://github.com/BlitterStudio/amiberry).
- Release approver: `@midwan`, who creates release tags and holds the signing certificate.

## Privacy

Amiberry does not transfer emulator configuration, ROMs, disk images, save data, or personal files to the Amiberry project.

Desktop builds can contact GitHub when the update-check feature is enabled or when the user starts an update/download action. Emulated networking only sends traffic when the user configures and runs networking inside the emulated Amiga environment.

The Android app privacy policy is published separately at [amiberry.com/privacy-policy](https://amiberry.com/privacy-policy).
