package com.blitterstudio.amiberry.overlay

import android.app.Activity
import android.hardware.input.InputManager
import android.os.Handler
import android.os.Looper
import android.view.InputDevice
import android.view.KeyEvent
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.platform.LocalContext
import com.blitterstudio.amiberry.ui.isTvDevice

/**
 * The TV remote as the Amiga's port 2 joystick. On a TV (leanback) with no
 * gamepad connected, the remote's D-pad moves the joystick in port 2 and OK
 * fires; Back still opens the menu. Plugging a gamepad in hands port 2 back
 * to the configuration's own device. The rule is applied at run time, so
 * configurations saved on a TV stay the same as anywhere else.
 *
 * The keys go to the emulator directly (OverlayNative.remoteKey), not
 * through SDL, which may take a remote for a joystick of its own; the
 * emulator gives them back while the classic settings window has the D-pad.
 */
object RemoteJoystick {
	/** Whether the remote drives port 2 now; the in-game menu's Controls page says so. */
	var isActive by mutableStateOf(false)
		internal set

	/** A gamepad or joystick; a remote is a D-pad and keys only. */
	fun isGamepad(sources: Int): Boolean =
		sources and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD ||
			sources and InputDevice.SOURCE_JOYSTICK == InputDevice.SOURCE_JOYSTICK

	/** Whether the remote drives port 2 on this device now: a TV with no gamepad connected. */
	fun drivesPort2(isTv: Boolean): Boolean = isTv && InputDevice.getDeviceIds().none { id ->
		InputDevice.getDevice(id)?.let { !it.isVirtual && isGamepad(it.sources) } == true
	}

	/** The joystick button a remote key stands for; 0 for any other key. */
	fun button(keyCode: Int): Int = when (keyCode) {
		KeyEvent.KEYCODE_DPAD_UP -> OverlayNative.REMOTE_UP
		KeyEvent.KEYCODE_DPAD_DOWN -> OverlayNative.REMOTE_DOWN
		KeyEvent.KEYCODE_DPAD_LEFT -> OverlayNative.REMOTE_LEFT
		KeyEvent.KEYCODE_DPAD_RIGHT -> OverlayNative.REMOTE_RIGHT
		KeyEvent.KEYCODE_DPAD_CENTER -> OverlayNative.REMOTE_FIRE
		else -> 0
	}
}

/**
 * [RemoteJoystick.drivesPort2] for the launcher, which runs apart from the emulator:
 * whether a launch now would have the remote drive port 2, kept up to date as
 * gamepads connect and disconnect.
 */
@Composable
fun rememberRemoteDrivesPort2(): Boolean {
	val context = LocalContext.current
	val isTv = isTvDevice()
	var drives by remember { mutableStateOf(RemoteJoystick.drivesPort2(isTv)) }
	if (isTv) {
		DisposableEffect(context) {
			val inputManager = context.getSystemService(InputManager::class.java)
			val listener = object : InputManager.InputDeviceListener {
				override fun onInputDeviceAdded(deviceId: Int) { drives = RemoteJoystick.drivesPort2(true) }
				override fun onInputDeviceRemoved(deviceId: Int) { drives = RemoteJoystick.drivesPort2(true) }
				override fun onInputDeviceChanged(deviceId: Int) { drives = RemoteJoystick.drivesPort2(true) }
			}
			inputManager?.registerInputDeviceListener(listener, Handler(Looper.getMainLooper()))
			onDispose { inputManager?.unregisterInputDeviceListener(listener) }
		}
	}
	return drives
}

/**
 * Follows the connected input devices for [RemoteJoystick] in
 * AmiberryActivity and hands it the remote's keys. Main thread.
 */
class RemoteJoystickHost(
	private val activity: Activity,
	/** Called when the remote starts driving port 2 (for the one-time hint). */
	private val onActivated: Runnable
) : InputManager.InputDeviceListener {
	private val isTv = isTvDevice(activity)
	private val inputManager = activity.getSystemService(InputManager::class.java)
	/** Remote keys whose press the emulator took; their releases go there too. */
	private val held = mutableSetOf<Int>()

	fun start() {
		if (!isTv) return
		inputManager?.registerInputDeviceListener(this, Handler(Looper.getMainLooper()))
		update()
	}

	fun stop() {
		if (!isTv) return
		inputManager?.unregisterInputDeviceListener(this)
	}

	/** A key the menu did not take; true when it moved the joystick. */
	fun onKeyEvent(event: KeyEvent): Boolean {
		val button = RemoteJoystick.button(event.keyCode)
		if (button == 0) return false
		return when (event.action) {
			KeyEvent.ACTION_DOWN -> when {
				event.keyCode in held -> true
				!RemoteJoystick.isActive || event.device?.let { RemoteJoystick.isGamepad(it.sources) } == true -> false
				OverlayNative.remoteKey(button, true) -> {
					held += event.keyCode
					true
				}
				else -> false
			}
			KeyEvent.ACTION_UP -> {
				if (!held.remove(event.keyCode)) return false
				OverlayNative.remoteKey(button, false)
				true
			}
			else -> false
		}
	}

	override fun onInputDeviceAdded(deviceId: Int) = update()
	override fun onInputDeviceRemoved(deviceId: Int) = update()
	override fun onInputDeviceChanged(deviceId: Int) = update()

	private fun update() {
		val active = RemoteJoystick.drivesPort2(isTv)
		if (active == RemoteJoystick.isActive) return
		RemoteJoystick.isActive = active
		OverlayNative.setRemoteJoystick(active)
		if (!active) held.clear()
		else onActivated.run()
	}
}
