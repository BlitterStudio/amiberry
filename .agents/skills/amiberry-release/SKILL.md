---
name: amiberry-release
description: >
  Workflow for preparing and publishing a new Amiberry release. Covers version bumps
  across CMakeLists.txt, vcpkg.json, metainfo.xml, flatpak manifest, RPM spec;
  commit/push/tag flow to trigger CI; generating release notes from git log;
  creating GitHub releases; and drafting social media announcements for Facebook,
  Mastodon, Bluesky, and Ko-Fi.
---

# Amiberry Release Workflow

## Pre-Release Checklist

### 1. Version Bump

Update the version string in ALL of these files:

| File | Field / Line | Notes |
|------|-------------|-------|
| `CMakeLists.txt` (root, line ~4) | `project(amiberry VERSION X.Y.Z)` | Single source of truth; Android derives from this |
| `vcpkg.json` | `"version": "X.Y.Z"` | Must match CMake |
| `packaging/linux/Amiberry.metainfo.xml` | Add new `<release>` entry at top of `<releases>` | Include date (YYYY-MM-DD) and `<description><p>` summary |
| `packaging/rpm/amiberry.spec` | `Version:` field + new `%changelog` entry | Use RPM date format: `* Thu Mar 26 2026 Dimitris Panokostas <midwan@gmail.com> - X.Y.Z-1` |
| `packaging/flatpak/com.blitterstudio.amiberry.yml` | `tag: vX.Y.Z` | The `commit:` hash is updated on the flathub repo, not here |

### 2. Commit, Push, Tag

```bash
git add packaging/flatpak/com.blitterstudio.amiberry.yml \
       packaging/linux/Amiberry.metainfo.xml \
       packaging/rpm/amiberry.spec \
       vcpkg.json
git commit -m "chore: update packaging version references for vX.Y.Z"
git push origin master
git tag vX.Y.Z
git push origin vX.Y.Z
```

The tag push triggers CI builds and artifact publishing via `.github/workflows/c-cpp.yml`.

### 3. Files NOT Needing Manual Update

- **Android version**: Derived automatically from `CMakeLists.txt` `project(VERSION)` via Gradle
- **Flatpak commit hash**: Updated on the flathub repo, not in this repo
- **DEB packaging**: Version derived from CMake at build time

## Release Notes

### Generate from Git Log

```bash
git log vPREVIOUS..vX.Y.Z --oneline --no-merges
```

### Format (follow existing convention)

```markdown
## Bug Fixes

- **Component:** Description of fix. (#PR_NUMBER)

## New Features

- **Component:** Description of feature. (#PR_NUMBER)

## Performance

- **Component:** Description of improvement.

## Other

- Housekeeping items, doc updates, etc.
```

### Guidelines

- Group by: Bug Fixes, New Features, Performance, Android, Other
- Bold the component/area name
- Reference PR numbers with `#NNN`
- Skip chore/docs/CI-only commits unless user-facing
- Keep descriptions concise — one line per item
- Write for end users, not developers (e.g., "Fixed crash when opening file dialog" not "prevent stale ImGuiListClipper context pointer")

## Social Media Posts

### Tone & Style

- Friendly, enthusiastic but not over-the-top
- Highlight 3-5 most impactful changes
- Include download link: `https://github.com/BlitterStudio/amiberry/releases/tag/vX.Y.Z`
- Mention it's available for Linux, macOS, Windows, and Android
- Use relevant hashtags: `#Amiga #Emulation #RetroComputing #Amiberry #OpenSource`

### Platform Specifics

| Platform | Max Length | Format Notes |
|----------|-----------|-------------|
| Facebook | ~63,000 chars | Longest format OK, can include full changelog link |
| Mastodon | 500 chars | Must be concise, use line breaks, hashtags at end |
| Bluesky | 300 chars | Most concise, link + 2-3 highlights max |
| Ko-Fi | ~10,000 chars | Can be detailed, thank supporters, mention what's next |

### Template Structure

**Facebook / Ko-Fi (longer):**
```
🎮 Amiberry vX.Y.Z is out!

[2-3 sentence summary of the release theme]

Highlights:
• [Feature/fix 1]
• [Feature/fix 2]
• [Feature/fix 3]
• [Feature/fix 4]

Download: [link]

Available for Linux, macOS, Windows and Android.

#Amiga #Emulation #RetroComputing #Amiberry #OpenSource
```

**Mastodon:**
```
🎮 Amiberry vX.Y.Z released!

[1 sentence summary]

Highlights:
• [Top 3 changes]

Download: [link]

#Amiga #Emulation #RetroComputing #Amiberry #OpenSource
```

**Bluesky:**
```
🎮 Amiberry vX.Y.Z is out! [1 sentence summary]. Download: [link]

#Amiga #Emulation #RetroComputing
```

## Post-Release

1. Monitor CI for build completion across all platforms
2. Once artifacts are uploaded, create the GitHub release (or verify auto-created)
3. Copy release notes to the GitHub release body
4. Post social media announcements
5. Check flathub repo picks up the new tag
