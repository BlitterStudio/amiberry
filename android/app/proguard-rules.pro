# Amiberry ProGuard / R8 Rules
# These rules ensure JNI, SDL, Compose, and Firebase classes survive minification.

# ── JNI ──────────────────────────────────────────────────────────────
# Keep all classes with native methods (JNI entry points)
-keepclasseswithmembernames class * {
    native <methods>;
}

# ── SDL ──────────────────────────────────────────────────────────────
# Keep SDL library classes (required by the native emulator via JNI reflection)
-keep class org.libsdl.app.** { *; }
-dontwarn org.libsdl.app.**

# ── Activities ───────────────────────────────────────────────────────
# Keep activities referenced in AndroidManifest.xml
-keep class com.blitterstudio.amiberry.ui.MainActivity { *; }
-keep class com.blitterstudio.amiberry.AmiberryActivity { *; }

# ── Data models ──────────────────────────────────────────────────────
# Keep data model classes (used in config parsing, file scanning, and serialization)
-keep class com.blitterstudio.amiberry.data.model.** { *; }

# ── ViewModels ───────────────────────────────────────────────────────
# Keep ViewModel constructors (instantiated by reflection via ViewModelProvider)
-keep class com.blitterstudio.amiberry.ui.viewmodel.** {
    <init>(...);
}

# ── Jetpack Compose ──────────────────────────────────────────────────
# Compose uses reflection for @Composable and state; the default R8 rules
# from the Compose BOM handle most cases, but keep Stability metadata.
-keepattributes RuntimeVisibleAnnotations,AnnotationDefault

# ── Firebase Crashlytics ─────────────────────────────────────────────
# Keep Crashlytics deobfuscation support
-keepattributes SourceFile,LineNumberTable
-renamesourcefileattribute SourceFile

# ── Kotlin ───────────────────────────────────────────────────────────
# Keep Kotlin metadata for reflection (used by serialization, ViewModels)
-keepattributes *Annotation*
-keep class kotlin.Metadata { *; }
-dontwarn kotlin.**
-dontwarn kotlinx.**

# ── General ──────────────────────────────────────────────────────────
# Keep enum values (used in AmigaModel, FileCategory, etc.)
-keepclassmembers enum * {
    public static **[] values();
    public static ** valueOf(java.lang.String);
}
