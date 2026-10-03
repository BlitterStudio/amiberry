# Plugin locations

Amiberry loads the first matching plugin file from these locations, in order:

1. The folder set in `AMIBERRY_PLUGINS_DIR`.
2. The optional **Plugins override** selected in Settings.
3. The per-user plugins folder.
4. The bundled or system plugins folder.

The per-user folder lets you add plugins without modifying an application bundle or a package-managed system directory:

- **macOS:** `~/Library/Application Support/Amiberry/Plugins`
- **Linux:** `$AMIBERRY_HOME_DIR/plugins`, or `~/Amiberry/plugins` when `AMIBERRY_HOME_DIR` is not set
- **Windows:** `%LOCALAPPDATA%\Amiberry\plugins`

Portable mode uses only its `plugins` folder.

## Installing a plugin

Open either **Paths** or **Global Settings**, then use **Open** to show the per-user folder or **Install plugin...** to copy a selected platform plugin library there. Installing a plugin never modifies the Amiberry app bundle or system package. If a file with the same name already exists, Amiberry asks before replacing it.

On macOS release builds, hardened-runtime library validation requires external plugins to be signed by Developer ID team `5GQP72592A`. Mac App Store builds are sandboxed and do not support external plugins.
