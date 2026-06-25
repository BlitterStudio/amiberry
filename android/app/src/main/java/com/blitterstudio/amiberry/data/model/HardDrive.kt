package com.blitterstudio.amiberry.data.model

/**
 * A hardfile (.hdf/.hdi/.vhd) mounted as an Amiga hard drive on the UAE
 * controller. Maps to a `hardfile2=` line in the .uae config.
 */
data class HardDrive(
	val path: String,
	val readOnly: Boolean = false,
	val bootPriority: Int = 0
)
