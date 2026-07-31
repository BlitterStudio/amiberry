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
	fun `captured D-pad motion has no distance release and shutdown neutralizes first`() {
		val joystick = File("../../src/osdep/on_screen_joystick.cpp").readText()
		val motionSignature =
			"bool on_screen_joystick_handle_finger_motion(const SDL_Event& event"
		val motionHandler = joystick.indexOf(motionSignature).let { start ->
			if (start >= 0) joystick.substring(start) else ""
		}
		val quitHandler = Regex(
			"""void on_screen_joystick_quit\(\)[\s\S]*?bool on_screen_joystick_is_enabled\(\)"""
		).find(joystick)?.value.orEmpty()
		val releaseAllHandler = Regex(
			"""void on_screen_joystick_release_all\(\)[\s\S]*?void on_screen_joystick_set_enabled"""
		).find(joystick)?.value.orEmpty()

		assertTrue(
			"Captured D-pad motion should continue updating direction at every distance.",
			motionHandler.contains("update_dpad_from_position(px, py)")
		)
		assertFalse(
			"The former distance-release threshold must be removed.",
			joystick.contains("DPAD_RELEASE_RADIUS")
		)
		assertFalse(
			"Motion must not neutralize or remove a captured owner.",
			motionHandler.contains("release_dpad()") ||
				motionHandler.contains("capture_registry.release(")
		)
		assertTrue(
			"Shutdown must delegate to the neutralization authority before texture teardown.",
			quitHandler.indexOf("on_screen_joystick_release_all()") in
				0 until quitHandler.indexOf("SDL_DestroyTexture")
		)
		assertTrue(
			"Global neutralization must clear direction, buttons, keyboard, knob, and capture state.",
			releaseAllHandler.contains("joy_up = joy_down = joy_left = joy_right = false") &&
				releaseAllHandler.contains("joy_fire1 = joy_fire2 = false") &&
				releaseAllHandler.contains("joy_kb_pressed = false") &&
				releaseAllHandler.contains("knob_active = false") &&
				releaseAllHandler.contains("capture_registry.clear()")
		)
	}

	@Test
	fun `touch cancellation and lifecycle seams use main-thread global neutralization`() {
		val amiberryCpp = File("../../src/osdep/amiberry.cpp").readText()
		val osk = File("../../src/osdep/imgui_osk.cpp").readText()
		val sdlCompat = File("../../libretro/sdl_compat.h").readText()
		val libretroStubs = File("../../libretro/libretro_gui_stubs.cpp").readText()
		val eventFilter = Regex(
			"""static bool SDLCALL android_touch_event_filter[\s\S]*?\R\}"""
		).find(amiberryCpp)?.value.orEmpty()
		val globalNeutralizer = Regex(
			"""static void neutralize_touch_controls\(\)[\s\S]*?\R\}"""
		).find(amiberryCpp)?.value.orEmpty()
		val inactiveHandler = Regex(
			"""static void amiberry_inactive[\s\S]*?void minimizewindow"""
		).find(amiberryCpp)?.value.orEmpty()
		val closeHandler = Regex(
			"""static void handle_close_event\(\)[\s\S]*?\R\}"""
		).find(amiberryCpp)?.value.orEmpty()
		val quitHandler = Regex(
			"""static void handle_quit_event\(\)[\s\S]*?\R\}"""
		).find(amiberryCpp)?.value.orEmpty()
		val processEvent = Regex(
			"""static void process_event\(const SDL_Event& event\)[\s\S]*?void update_clipboard"""
		).find(amiberryCpp)?.value.orEmpty()

		assertTrue(
			"The Android SDL event filter should only publish an atomic pending-neutralization request.",
			eventFilter.contains("android_touch_neutralization_pending.store(true") &&
				!eventFilter.contains("on_screen_joystick_release_all()") &&
				!eventFilter.contains("neutralize_touch_controls()")
		)
		assertTrue(
			"The event filter must be installed for Android lifecycle delivery.",
			amiberryCpp.contains("SDL_SetEventFilter(android_touch_event_filter, nullptr)")
		)
		assertTrue(
			"Main-thread event processing must drain pending neutralization before dispatching resumed or input events.",
			processEvent.indexOf("drain_pending_touch_neutralization()") in
				0 until processEvent.indexOf("switch (event.type)")
		)
		assertTrue(
			"The global authority must release every overlay owner and reset both Android swipe slots.",
			globalNeutralizer.contains("on_screen_joystick_release_all()") &&
				globalNeutralizer.contains("reset_android_two_finger_swipe()")
		)
		assertTrue(
			"Finger cancellation must neutralize globally before ordinary per-finger routing.",
			processEvent.indexOf("case SDL_EVENT_FINGER_CANCELED:") in
				0 until processEvent.indexOf("case SDL_EVENT_FINGER_DOWN:") &&
				Regex("""case SDL_EVENT_FINGER_CANCELED:\s*neutralize_touch_controls\(\);\s*break;""")
					.containsMatchIn(processEvent)
		)
		assertTrue(
			"Focus loss and minimization must neutralize before input unacquire.",
			inactiveHandler.indexOf("neutralize_touch_controls()") in
				0 until inactiveHandler.indexOf("inputdevice_unacquire")
		)
		assertTrue(
			"Background must neutralize before pausing, and foreground before resuming.",
			Regex("""case SDL_EVENT_DID_ENTER_BACKGROUND:\s*neutralize_touch_controls\(\);\s*pause_sound\(\);""")
				.containsMatchIn(processEvent) &&
				Regex("""case SDL_EVENT_DID_ENTER_FOREGROUND:\s*neutralize_touch_controls\(\);\s*pause_emulation = 0;\s*resume_sound\(\);""")
					.containsMatchIn(processEvent)
		)
		assertTrue(
			"Quit, close, and termination must neutralize before teardown.",
			closeHandler.indexOf("neutralize_touch_controls()") in
				0 until closeHandler.indexOf("inputdevice_unacquire") &&
				quitHandler.indexOf("neutralize_touch_controls()") in
					0 until quitHandler.indexOf("uae_quit()") &&
				processEvent.contains("case SDL_EVENT_TERMINATING:")
		)
		assertTrue(
			"Opening the OSK must preserve its existing joystick-release takeover path.",
			Regex("""void imgui_osk_toggle\(\)[\s\S]*on_screen_joystick_release_all\(\)[\s\S]*reset_navigation_state\(\)""")
				.containsMatchIn(osk)
		)
		assertTrue(
			"Libretro SDL compatibility must mirror canceled and terminating events.",
			sdlCompat.contains("SDL_EVENT_FINGER_CANCELED") &&
				sdlCompat.contains("SDL_EVENT_TERMINATING")
		)
		assertTrue(
			"The headless libretro layer must provide the global release API.",
			libretroStubs.contains("void on_screen_joystick_release_all() {}")
		)
	}

	@Test
	fun `only unconsumed paired touch fingers participate in Android GUI swipe`() {
		val amiberryCpp = File("../../src/osdep/amiberry.cpp").readText()
		val swipeTracker = Regex(
			"""struct AndroidSwipeFinger[\s\S]*?#endif"""
		).find(amiberryCpp)?.value.orEmpty()
		val fingerEvents = Regex(
			"""case SDL_EVENT_FINGER_DOWN:[\s\S]*?case SDL_EVENT_MOUSE_BUTTON_DOWN:"""
		).find(amiberryCpp)?.value.orEmpty()
		val fingerMotion = Regex(
			"""case SDL_EVENT_FINGER_MOTION:[\s\S]*?case SDL_EVENT_MOUSE_MOTION:"""
		).find(amiberryCpp)?.value.orEmpty()
		val firstTouch = Pair(101L, 7L)
		val secondTouch = Pair(202L, 7L)

		assertFalse(
			"Identical valid finger IDs on different valid touch devices are distinct swipe owners.",
			firstTouch == secondTouch
		)
		assertTrue(
			"Swipe slots must use explicit occupancy and paired touch/finger identifiers.",
			swipeTracker.contains("bool occupied") &&
				swipeTracker.contains("SDL_TouchID touch_id") &&
				swipeTracker.contains("SDL_FingerID finger_id") &&
				swipeTracker.contains("slot.touch_id == touch_id") &&
				swipeTracker.contains("slot.finger_id == finger_id")
		)
		assertFalse(
			"Swipe occupancy must not use a zero identifier sentinel.",
			swipeTracker.contains("finger_ids[0] == 0") ||
				swipeTracker.contains("finger_ids[slot] = 0")
		)
		assertTrue(
			"Down/up and motion events may reach the GUI swipe tracker only when overlays did not consume them.",
			Regex("""if \(!consumed\)\s*handle_android_two_finger_swipe\(event\);""")
				.containsMatchIn(fingerEvents) &&
				Regex("""if \(!consumed\)\s*handle_android_two_finger_swipe\(event\);""")
					.containsMatchIn(fingerMotion)
		)
		assertTrue(
			"Two ordinary unconsumed fingers must retain the existing downward-swipe GUI action.",
			swipeTracker.contains("android_swipe_finger_count() != 2") &&
				swipeTracker.contains("inputdevice_add_inputcode(AKS_ENTERGUI, 1, nullptr)")
		)
		assertTrue(
			"Global swipe reset must clear both occupied slots and the triggered state.",
			swipeTracker.contains("static void reset_android_two_finger_swipe()") &&
				swipeTracker.contains("slot = {}") &&
				swipeTracker.contains("android_swipe_triggered = false")
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
		val controllerManager = File("src/main/java/org/libsdl/app/SDLControllerManager.java").readText()
		val mouseBranch = Regex(
			"""if \(toolType == MotionEvent\.TOOL_TYPE_MOUSE\) \{([\s\S]*?)\R\s*\} else if \(toolType == MotionEvent\.TOOL_TYPE_STYLUS"""
		).find(surface)?.groupValues?.get(1).orEmpty()
		val genericMouseBranch = Regex(
			"""if \(toolType == MotionEvent\.TOOL_TYPE_MOUSE\) \{([\s\S]*?)\R\s*\} else if \(toolType == MotionEvent\.TOOL_TYPE_STYLUS"""
		).find(controllerManager)?.groupValues?.get(1).orEmpty()

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
		assertTrue(
			"Mouse lifecycle ACTION_DOWN/ACTION_UP should remain as a fallback for devices that do not deliver explicit button transitions.",
			mouseBranch.contains("case MotionEvent.ACTION_DOWN:") &&
				mouseBranch.contains("case MotionEvent.ACTION_UP:") &&
				mouseBranch.contains("mMouseLifecycleButtonActive")
		)
		assertTrue(
			"Lifecycle mouse fallback should be suppressed while an explicit button transition owns the press.",
			mouseBranch.contains("mouseButtonTransitionActive()")
		)
		assertFalse(
			"Mouse tool ACTION_DOWN/ACTION_UP lifecycle events should not be forwarded directly as button transitions.",
			mouseBranch.contains("SDLActivity.onNativeMouse(buttonState, action, x, y, relative)")
		)
		assertTrue(
			"Generic motion should forward explicit mouse button press/release events before touch lifecycle fallback sees them.",
			genericMouseBranch.contains("case MotionEvent.ACTION_BUTTON_PRESS:") &&
				genericMouseBranch.contains("case MotionEvent.ACTION_BUTTON_RELEASE:") &&
				genericMouseBranch.contains("SDLSurface.beginMouseButtonTransition(event.getButtonState())") &&
				genericMouseBranch.contains("SDLSurface.finishMouseButtonTransition(event.getButtonState())")
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

	@Test
	fun `Android SDL Java shim includes SDL 3_4_12 reliability fixes`() {
		val hidDeviceUsb = File("src/main/java/org/libsdl/app/HIDDeviceUSB.java").readText()
		val surface = File("src/main/java/org/libsdl/app/SDLSurface.java").readText()
		val sensorManager = File("src/main/java/org/libsdl/app/SDLSensorManager.java").readText()

		assertTrue(
			"USB serial lookup should tolerate every exception handled by SDL 3.4.12.",
			hidDeviceUsb.contains("catch (Exception exception)")
		)
		assertFalse(
			"USB serial lookup should not remain limited to SecurityException.",
			hidDeviceUsb.contains("catch (SecurityException exception)")
		)
		assertTrue(
			"SDLSurface should route sensor registration through SDL's synchronized retry wrapper.",
			surface.contains("SDLSensorManager.registerListener(mSensorManager, this,") &&
				surface.contains("SDLSensorManager.unregisterListener(mSensorManager, this,")
		)
		assertTrue(
			"The SDL sensor wrapper should retry both registration paths after ConcurrentModificationException.",
			sensorManager.contains("static final int RETRY_COUNT = 3;") &&
				Regex("""catch \(java\.util\.ConcurrentModificationException e\)""")
					.findAll(sensorManager)
					.count() == 2
		)
	}
}
