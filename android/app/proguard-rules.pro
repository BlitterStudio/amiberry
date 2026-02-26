# Amiberry ProGuard Rules
# These rules ensure JNI and SDL classes are preserved when minification is enabled.

# Keep all classes with native methods (JNI entry points)
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep SDL library classes (required by the native emulator)
-keep class org.libsdl.app.** { *; }

# Keep data model classes (used in config parsing and file scanning)
-keep class com.blitterstudio.amiberry.data.model.** { *; }

# Keep activities referenced in AndroidManifest.xml
-keep class com.blitterstudio.amiberry.ui.MainActivity { *; }
-keep class com.blitterstudio.amiberry.AmiberryActivity { *; }

# Don't warn about missing SDL references (native-only paths)
-dontwarn org.libsdl.app.**
