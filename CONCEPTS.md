# Amiberry Concepts

## Host display

### Shared-window mode

An Amiberry host-display mode where emulation and the settings GUI reuse one native window because separate windows cannot be managed reliably in the active environment.

Only one owner presents through the shared window at a time. A transition between the GUI and emulation must restore the next owner's display state and complete any presentation work that cannot safely cross the ownership boundary.

### Presentation rectangle

The host-display region where the emulated frame is finally drawn after aspect correction, scaling, centering, and any active bezel constraint have been resolved.

The presentation rectangle may be smaller than the full drawable area, or may extend beyond it when an unbounded centered mode preserves a requested source size.

### Bezel hole

The transparent screen region inside a custom bezel that can replace the full drawable area as the bounded region for emulated-frame presentation.

## Touch input

### Captured joystick gesture

An on-screen joystick gesture that owns directional input from an initial touch inside its acquisition area until finger-up, platform cancellation, application focus loss, or control shutdown.

Moving the held finger outside the visible joystick base continues to update direction and does not release or recenter the gesture.
