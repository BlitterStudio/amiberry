# Amiberry Concepts

## Host display

### Shared-window mode

An Amiberry host-display mode where emulation and the settings GUI reuse one native window because separate windows cannot be managed reliably in the active environment.

Only one owner presents through the shared window at a time. A transition between the GUI and emulation must restore the next owner's display state and complete any presentation work that cannot safely cross the ownership boundary.
