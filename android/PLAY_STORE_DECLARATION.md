# Play Store: MANAGE_EXTERNAL_STORAGE Declaration

> Use this text when submitting the "All files access" permission declaration
> in Google Play Console → Policy and programs → App content → Sensitive permissions.

---

## Permission: MANAGE_EXTERNAL_STORAGE

### Core Functionality

Amiberry is an Amiga computer emulator that requires access to user-provided binary files (ROM images, floppy disk images, hard disk images, CD images, and WHDLoad game archives) to function. These files are the digital equivalent of physical media from the original Amiga hardware platform.

### Why Scoped Storage Is Insufficient

1. **User-managed file collections**: Amiga enthusiasts maintain large, organized collections of disk images and ROM files in directories of their choosing — often on external SD cards or shared storage accessible to multiple apps (file managers, torrent clients, cloud sync tools). Requiring users to import/copy these files into the app's scoped directory would:
   - Double storage consumption (collections commonly exceed several gigabytes)
   - Break existing file organization workflows
   - Prevent real-time access to files synced or downloaded by other apps

2. **File format diversity**: The emulator handles 20+ binary file formats (.adf, .ipf, .dms, .hdf, .lha, .lzx, .lzh, .iso, .cue, .bin, .chd, .uae, .rom, .a500, .a1200, .zip, .7z, .gz, .xz, .rp9) with no registered MIME types. The Storage Access Framework's document picker cannot reliably filter these formats, degrading the user experience.

3. **Configuration file references**: Emulator configuration files (.uae) contain absolute paths to disk images, ROM files, and hard drive directories. These paths must resolve at runtime. Scoped storage would break all existing configurations when files are copied to a different location.

4. **WHDLoad game directories**: The WHDLoad system (Amiga game installer) creates directory structures with specific path expectations. These must be accessible in-place, not copied into scoped storage.

### User Consent Flow

- On first launch, the app displays the system permission dialog explaining why all-files access is needed
- If the user denies permission, the app continues to function using only its scoped storage directory
- The permission is requested exactly once; the app does not repeatedly prompt

### Data Access Scope

The app only reads/writes:
- Amiga ROM image files (for system emulation)
- Floppy/hard disk/CD image files (for media emulation)
- WHDLoad game archives (.lha)
- Emulator configuration files (.uae)
- Savestate files (.uss)

The app does **not** access contacts, photos, documents, or any non-emulation files.

### Category

File management / Utility — the app needs to manage user-provided binary files that exist outside scoped storage boundaries.

---

## Short-Form Declaration (for Play Console text field)

Amiberry is an Amiga emulator requiring access to user-provided ROM images, disk images, and game archives in user-chosen directories. Files use 20+ formats (.adf, .ipf, .hdf, .lha, .iso, etc.) with no registered MIME types, making SAF impractical. Config files (.uae) reference absolute paths that break if copied. Users maintain multi-GB collections that cannot be duplicated into scoped storage. Permission is requested once; if denied, the app uses scoped storage only.
