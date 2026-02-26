# Amiberry ProGuard Rules
# These rules ensure JNI and SDL classes are preserved when minification is enabled.

# Keep all classes with native methods (JNI entry points)
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep SDL library classes (required by the native emulator)
-keep class org.libsdl.app.** { *; }

# Keep all Amiberry application classes
-keep class com.blitterstudio.amiberry.** { *; }

# Keep Compose-related classes that may be referenced via reflection
-keep class androidx.compose.** { *; }

# Don't warn about missing SDL references (native-only paths)
-dontwarn org.libsdl.app.**
