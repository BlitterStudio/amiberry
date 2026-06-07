package com.blitterstudio.amiberry.ui

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

class AndroidRuntimeControlsArchitectureTest {

	@Test
	fun `SDL activity routes legacy and predictive Back through SDL`() {
		val activity = File("src/main/java/com/blitterstudio/amiberry/AmiberryActivity.java")
			.readText()

		val onBackPressed = Regex("""public void onBackPressed\(\) \{[\s\S]*?\n\t\}""")
			.find(activity)
			?.value
			.orEmpty()
		val registerBackCallback = Regex("""private void registerBackCallback\(\) \{[\s\S]*?\n\t\}""")
			.find(activity)
			?.value
			.orEmpty()
		val dispatchBackToSdl = Regex("""private void dispatchBackToSdl\(\) \{[\s\S]*?\n\t\}""")
			.find(activity)
			?.value
			.orEmpty()

		assertTrue(
			"Legacy Back should be translated into the same SDL key path as predictive Back.",
			onBackPressed.contains("dispatchBackToSdl()")
		)
		assertTrue(
			"Predictive Back should register the same SDL dispatch callback on API 33+.",
			registerBackCallback.contains("backCallback = this::dispatchBackToSdl")
		)
		assertTrue(
			"Back dispatch should synthesize a complete KEYCODE_BACK down/up pair for SDL.",
			Regex("""onNativeKeyDown\(KeyEvent\.KEYCODE_BACK\)[\s\S]*onNativeKeyUp\(KeyEvent\.KEYCODE_BACK\)""")
				.containsMatchIn(dispatchBackToSdl)
		)
		assertFalse(
			"The activity comment should describe the Java bridge instead of claiming Java-side Back handling is unnecessary.",
			activity.contains("No Java-side back handling is needed")
		)
	}

	@Test
	fun `manifest enables predictive Back callback for SDL activity`() {
		val manifest = File("src/main/AndroidManifest.xml").readText()

		assertTrue(
			"AmiberryActivity should opt into predictive Back so API 33+ can route Back through its callback.",
			Regex("""<activity android:name="com\.blitterstudio\.amiberry\.AmiberryActivity"[\s\S]*?android:enableOnBackInvokedCallback="true"""")
				.containsMatchIn(manifest)
		)
	}

	@Test
	fun `Android Back closes the OSK before opening the emulator GUI`() {
		val amiberryCpp = File("../../src/osdep/amiberry.cpp").readText()
		val backHandler = Regex("""if \(event\.key\.scancode == SDL_SCANCODE_AC_BACK\) \{[\s\S]*?\n\t\}""")
			.find(amiberryCpp)
			?.value
			.orEmpty()

		assertTrue(
			"Android Back should handle SDL_SCANCODE_AC_BACK explicitly.",
			backHandler.isNotEmpty()
		)
		assertTrue(
			"Back should hide the on-screen keyboard before using Back as the GUI shortcut.",
			Regex("""if \(imgui_osk_should_render\(\)\)[\s\S]*imgui_osk_hide\(\);[\s\S]*else[\s\S]*AKS_ENTERGUI""")
				.containsMatchIn(backHandler)
		)
	}

	@Test
	fun `touch overlays block joystick and mouse input while OSK is animating`() {
		val amiberryCpp = File("../../src/osdep/amiberry.cpp").readText()
		val fingerEvents = Regex(
			"""case SDL_EVENT_FINGER_DOWN:[\s\S]*?case SDL_EVENT_MOUSE_WHEEL:"""
		).find(amiberryCpp)?.value.orEmpty()
		val controllerButtonEvent = Regex(
			"""static void handle_controller_button_event\(const SDL_Event& event\)[\s\S]*?static void handle_joy_button_event"""
		).find(amiberryCpp)?.value.orEmpty()

		assertTrue(
			"OSK touch routing should stay active for the visible and animating keyboard lifetime.",
			fingerEvents.contains("imgui_osk_should_render() && mon->amiga_window")
		)
		assertTrue(
			"The on-screen joystick should stay blocked until the OSK has finished sliding out.",
			fingerEvents.contains("!imgui_osk_should_render() && on_screen_joystick_is_enabled()")
		)
		assertTrue(
			"Touch-synthesized mouse events should be suppressed while the OSK is visible or animating.",
			fingerEvents.contains("imgui_osk_should_render() || on_screen_joystick_is_enabled()")
		)
		assertTrue(
			"Gamepad OSK navigation should consume controller input while the keyboard is still animating.",
			controllerButtonEvent.contains("else if (imgui_osk_should_render())") &&
				controllerButtonEvent.contains("if (!imgui_osk_is_active())")
		)
	}

	@Test
	fun `OSK hit testing includes the animated keyboard rectangle`() {
		val osk = File("../../src/osdep/imgui_osk.cpp").readText()
		val hitTest = Regex("""bool imgui_osk_hit_test\(float screen_x, float screen_y\)\R\{[\s\S]*?\R\}""")
			.find(osk)
			?.value
			.orEmpty()

		assertTrue(
			"OSK hit testing should consume input while the keyboard is visible or animating.",
			hitTest.contains("imgui_osk_should_render()")
		)
		assertTrue(
			"OSK hit testing should account for slide-in and slide-out offsets.",
			hitTest.contains("s_kb_h * (1.0f - s_anim_progress)") &&
				hitTest.contains("s_kb_h * s_anim_progress")
		)
		assertFalse(
			"OSK hit testing should not only work after the keyboard is fully visible.",
			hitTest.contains("s_anim_progress < 1.0f")
		)
	}

	@Test
	fun `Android physical mouse buttons use button transitions instead of touch lifecycle actions`() {
		val surface = File("src/main/java/org/libsdl/app/SDLSurface.java").readText()
		val mouseBranch = Regex(
			"""if \(toolType == MotionEvent\.TOOL_TYPE_MOUSE\) \{([\s\S]*?)\R\s*\} else if \(toolType == MotionEvent\.TOOL_TYPE_STYLUS"""
		).find(surface)?.groupValues?.get(1).orEmpty()

		assertTrue(
			"SDLSurface.onTouch() should keep a distinct TOOL_TYPE_MOUSE branch.",
			mouseBranch.isNotEmpty()
		)
		assertTrue(
			"Physical mouse button input should be handled from ACTION_BUTTON_PRESS/RELEASE so ChromeOS long-presses do not see lifecycle ACTION_UP as a release.",
			mouseBranch.contains("case MotionEvent.ACTION_BUTTON_PRESS:") &&
				mouseBranch.contains("case MotionEvent.ACTION_BUTTON_RELEASE:")
		)
		assertTrue(
			"SDL expects mouse button transitions as ACTION_DOWN/ACTION_UP with the current Android button-state mask.",
			mouseBranch.contains("SDLActivity.onNativeMouse(buttonState, MotionEvent.ACTION_DOWN, x, y, relative)") &&
				mouseBranch.contains("SDLActivity.onNativeMouse(buttonState, MotionEvent.ACTION_UP, x, y, relative)")
		)
		assertFalse(
			"Mouse tool ACTION_DOWN/ACTION_UP lifecycle events should not be forwarded directly as button transitions.",
			mouseBranch.contains("SDLActivity.onNativeMouse(buttonState, action, x, y, relative)")
		)
	}

	@Test
	fun `Android SDL Java shim matches SDL 3_4 pen JNI contract`() {
		val activity = File("src/main/java/org/libsdl/app/SDLActivity.java").readText()
		val surface = File("src/main/java/org/libsdl/app/SDLSurface.java").readText()
		val controllerManager = File("src/main/java/org/libsdl/app/SDLControllerManager.java").readText()

		assertTrue(
			"SDL 3.4 expects onNativePen to include the pen device type before button state.",
			activity.contains(
				"public static native void onNativePen(int penId, int device_type, int button, int action, float x, float y, float p);"
			)
		)
		assertTrue(
			"Surface pen events should pass the SDL motion listener's pen device type to native SDL.",
			surface.contains(
				"SDLActivity.onNativePen(pointerId, SDLActivity.getMotionListener().getPenDeviceType(event.getDevice()), buttonState, action, x, y, p);"
			)
		)
		assertTrue(
			"Generic pen hover events should pass the pen device type to native SDL.",
			controllerManager.contains(
				"SDLActivity.onNativePen(event.getPointerId(i), getPenDeviceType(event.getDevice()), buttons, action, x, y, p);"
			)
		)
		assertTrue(
			"API 29+ can distinguish direct pens from indirect external tablets for SDL 3.4.",
			controllerManager.contains("class SDLGenericMotionListener_API29 extends SDLGenericMotionListener_API26") &&
				controllerManager.contains("return penDevice.isExternal() ? SDL_PEN_DEVICE_TYPE_INDIRECT : SDL_PEN_DEVICE_TYPE_DIRECT;")
		)
		assertTrue(
			"SDLActivity should use the API 29 motion listener when available.",
			activity.contains("mMotionListener = new SDLGenericMotionListener_API29();")
		)
	}
}
