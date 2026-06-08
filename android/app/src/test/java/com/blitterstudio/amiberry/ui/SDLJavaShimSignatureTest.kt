package com.blitterstudio.amiberry.ui

import android.app.Activity
import android.view.Surface
import org.junit.Assert.assertEquals
import org.junit.Test
import org.libsdl.app.HIDDeviceManager
import org.libsdl.app.SDLAudioManager
import org.libsdl.app.SDLActivity
import org.libsdl.app.SDLControllerManager
import org.libsdl.app.SDLInputConnection

class SDLJavaShimSignatureTest {
	@Test
	fun nativeSetupJniMethodsMatchSdlRegistration() {
		assertNativeSetupJniReturnsVoid(SDLActivity::class.java)
		assertNativeSetupJniReturnsVoid(SDLAudioManager::class.java)
		assertNativeSetupJniReturnsVoid(SDLControllerManager::class.java)
	}

	@Test
	fun sdlActivityVersionMatchesVendoredSdl() {
		assertEquals(3, SDLActivity::class.java.getPrivateInt("SDL_MAJOR_VERSION"))
		assertEquals(4, SDLActivity::class.java.getPrivateInt("SDL_MINOR_VERSION"))
		assertEquals(10, SDLActivity::class.java.getPrivateInt("SDL_MICRO_VERSION"))
	}

	@Test
	fun registeredNativeMethodsMatchSdlRegistrationTables() {
		assertNativeMethods(
			SDLActivity::class.java,
			listOf(
				NativeSignature("nativeGetVersion", stringType),
				NativeSignature("nativeSetupJNI", voidType),
				NativeSignature("nativeInitMainThread", voidType),
				NativeSignature("nativeCleanupMainThread", voidType),
				NativeSignature("nativeRunMain", intType, stringType, stringType, objectType),
				NativeSignature("onNativeDropFile", voidType, stringType),
				NativeSignature("nativeSetScreenResolution", voidType, intType, intType, intType, intType, floatType, floatType),
				NativeSignature("onNativeResize", voidType),
				NativeSignature("onNativeSurfaceCreated", voidType),
				NativeSignature("onNativeSurfaceChanged", voidType),
				NativeSignature("onNativeSurfaceDestroyed", voidType),
				NativeSignature("onNativeScreenKeyboardShown", voidType),
				NativeSignature("onNativeScreenKeyboardHidden", voidType),
				NativeSignature("onNativeKeyDown", voidType, intType),
				NativeSignature("onNativeKeyUp", voidType, intType),
				NativeSignature("onNativeSoftReturnKey", boolType),
				NativeSignature("onNativeKeyboardFocusLost", voidType),
				NativeSignature("onNativeTouch", voidType, intType, intType, intType, floatType, floatType, floatType),
				NativeSignature("onNativePinchStart", voidType),
				NativeSignature("onNativePinchUpdate", voidType, floatType),
				NativeSignature("onNativePinchEnd", voidType),
				NativeSignature("onNativeMouse", voidType, intType, intType, floatType, floatType, boolType),
				NativeSignature("onNativePen", voidType, intType, intType, intType, intType, floatType, floatType, floatType),
				NativeSignature("onNativeAccel", voidType, floatType, floatType, floatType),
				NativeSignature("onNativeClipboardChanged", voidType),
				NativeSignature("nativeLowMemory", voidType),
				NativeSignature("onNativeLocaleChanged", voidType),
				NativeSignature("onNativeDarkModeChanged", voidType, boolType),
				NativeSignature("nativeSendQuit", voidType),
				NativeSignature("nativeQuit", voidType),
				NativeSignature("nativePause", voidType),
				NativeSignature("nativeResume", voidType),
				NativeSignature("nativeFocusChanged", voidType, boolType),
				NativeSignature("nativeGetHint", stringType, stringType),
				NativeSignature("nativeGetHintBoolean", boolType, stringType, boolType),
				NativeSignature("nativeSetenv", voidType, stringType, stringType),
				NativeSignature("nativeSetNaturalOrientation", voidType, intType),
				NativeSignature("onNativeRotationChanged", voidType, intType),
				NativeSignature("onNativeInsetsChanged", voidType, intType, intType, intType, intType),
				NativeSignature("nativeAddTouch", voidType, intType, stringType),
				NativeSignature("nativePermissionResult", voidType, intType, boolType),
				NativeSignature("nativeAllowRecreateActivity", boolType),
				NativeSignature("nativeCheckSDLThreadCounter", intType),
				NativeSignature("onNativeFileDialog", voidType, intType, stringArrayType, intType),
			)
		)
		assertNativeMethods(
			SDLInputConnection::class.java,
			listOf(
				NativeSignature("nativeCommitText", voidType, stringType, intType),
				NativeSignature("nativeGenerateScancodeForUnichar", voidType, charType),
			)
		)
		assertNativeMethods(
			SDLAudioManager::class.java,
			listOf(
				NativeSignature("nativeSetupJNI", voidType),
				NativeSignature("nativeAddAudioDevice", voidType, boolType, stringType, intType),
				NativeSignature("nativeRemoveAudioDevice", voidType, boolType, intType),
			)
		)
		assertNativeMethods(
			SDLControllerManager::class.java,
			listOf(
				NativeSignature("nativeSetupJNI", voidType),
				NativeSignature("onNativePadDown", boolType, intType, intType, intType),
				NativeSignature("onNativePadUp", boolType, intType, intType, intType),
				NativeSignature("onNativeJoy", voidType, intType, intType, floatType),
				NativeSignature("onNativeHat", voidType, intType, intType, intType, intType),
				NativeSignature("onNativeJoySensor", voidType, intType, intType, longType, floatType, floatType, floatType),
				NativeSignature(
					"nativeAddJoystick",
					voidType,
					intType,
					stringType,
					stringType,
					intType,
					intType,
					intType,
					intType,
					intType,
					intType,
					boolType,
					boolType,
					boolType,
					boolType
				),
				NativeSignature("nativeRemoveJoystick", voidType, intType),
				NativeSignature("nativeAddHaptic", voidType, intType, stringType),
				NativeSignature("nativeRemoveHaptic", voidType, intType),
			)
		)
		assertNativeMethods(
			HIDDeviceManager::class.java,
			listOf(
				NativeSignature("HIDDeviceRegisterCallback", voidType),
				NativeSignature("HIDDeviceReleaseCallback", voidType),
				NativeSignature(
					"HIDDeviceConnected",
					voidType,
					intType,
					stringType,
					intType,
					intType,
					stringType,
					intType,
					stringType,
					stringType,
					intType,
					intType,
					intType,
					intType,
					boolType,
					intType
				),
				NativeSignature("HIDDeviceOpenPending", voidType, intType),
				NativeSignature("HIDDeviceOpenResult", voidType, intType, boolType),
				NativeSignature("HIDDeviceDisconnected", voidType, intType),
				NativeSignature("HIDDeviceInputReport", voidType, intType, byteArrayType),
				NativeSignature("HIDDeviceReportResponse", voidType, intType, byteArrayType),
			)
		)
	}

	@Test
	fun sdlSetupCallbackMethodsMatchSdlLookupTables() {
		assertNativeMethods(
			SDLActivity::class.java,
			listOf(
				NativeSignature("clipboardGetText", stringType),
				NativeSignature("clipboardHasText", boolType),
				NativeSignature("clipboardSetText", voidType, stringType),
				NativeSignature("createCustomCursor", intType, intArrayType, intType, intType, intType, intType),
				NativeSignature("destroyCustomCursor", voidType, intType),
				NativeSignature("getContext", activityType),
				NativeSignature("getManifestEnvironmentVariables", boolType),
				NativeSignature("getNativeSurface", surfaceType),
				NativeSignature("initTouch", voidType),
				NativeSignature("isAndroidTV", boolType),
				NativeSignature("isChromebook", boolType),
				NativeSignature("isDeXMode", boolType),
				NativeSignature("isTablet", boolType),
				NativeSignature("manualBackButton", voidType),
				NativeSignature("minimizeWindow", voidType),
				NativeSignature("openURL", boolType, stringType),
				NativeSignature("requestPermission", voidType, stringType, intType),
				NativeSignature("showToast", boolType, stringType, intType, intType, intType, intType),
				NativeSignature("sendMessage", boolType, intType, intType),
				NativeSignature("setActivityTitle", boolType, stringType),
				NativeSignature("setCustomCursor", boolType, intType),
				NativeSignature("setOrientation", voidType, intType, intType, boolType, stringType),
				NativeSignature("setRelativeMouseEnabled", boolType, boolType),
				NativeSignature("setSystemCursor", boolType, intType),
				NativeSignature("setWindowStyle", voidType, boolType),
				NativeSignature("shouldMinimizeOnFocusLoss", boolType),
				NativeSignature("showTextInput", boolType, intType, intType, intType, intType, intType),
				NativeSignature("supportsRelativeMouse", boolType),
				NativeSignature("openFileDescriptor", intType, stringType, stringType),
				NativeSignature("showFileDialog", boolType, stringArrayType, boolType, boolType, intType),
				NativeSignature("getPreferredLocales", stringType),
			)
		)
		assertNativeMethods(
			SDLAudioManager::class.java,
			listOf(
				NativeSignature("registerAudioDeviceCallback", voidType),
				NativeSignature("unregisterAudioDeviceCallback", voidType),
				NativeSignature("audioSetThreadPriority", voidType, boolType, intType),
			)
		)
		assertNativeMethods(
			SDLControllerManager::class.java,
			listOf(
				NativeSignature("pollInputDevices", voidType),
				NativeSignature("joystickSetLED", voidType, intType, intType, intType, intType),
				NativeSignature("joystickSetSensorsEnabled", voidType, intType, boolType),
				NativeSignature("pollHapticDevices", voidType),
				NativeSignature("hapticRun", voidType, intType, floatType, intType),
				NativeSignature("hapticRumble", voidType, intType, floatType, floatType, intType),
				NativeSignature("hapticStop", voidType, intType),
			)
		)
	}

	private fun assertNativeSetupJniReturnsVoid(clazz: Class<*>) {
		val method = clazz.getDeclaredMethod("nativeSetupJNI")
		assertEquals("nativeSetupJNI return type for ${clazz.name}", Void.TYPE, method.returnType)
	}

	private fun assertNativeMethods(clazz: Class<*>, signatures: List<NativeSignature>) {
		for (signature in signatures) {
			val method = clazz.getDeclaredMethod(signature.name, *signature.parameterTypes)
			assertEquals("return type for ${clazz.name}.${signature.name}", signature.returnType, method.returnType)
		}
	}

	private fun Class<*>.getPrivateInt(fieldName: String): Int {
		val field = getDeclaredField(fieldName)
		field.isAccessible = true
		return field.getInt(null)
	}

	private class NativeSignature(
		val name: String,
		val returnType: Class<*>,
		vararg val parameterTypes: Class<*>
	)

	private companion object {
		val voidType: Class<*> = Void.TYPE
		val boolType: Class<*> = Boolean::class.javaPrimitiveType!!
		val charType: Class<*> = Char::class.javaPrimitiveType!!
		val floatType: Class<*> = Float::class.javaPrimitiveType!!
		val activityType: Class<*> = Activity::class.java
		val byteArrayType: Class<*> = ByteArray::class.java
		val intType: Class<*> = Int::class.javaPrimitiveType!!
		val intArrayType: Class<*> = IntArray::class.java
		val longType: Class<*> = Long::class.javaPrimitiveType!!
		val objectType: Class<*> = Any::class.java
		val surfaceType: Class<*> = Surface::class.java
		val stringType: Class<*> = String::class.java
		val stringArrayType: Class<*> = Array<String>::class.java
	}
}
