package com.blitterstudio.amiberry.data.model

/**
 * Amiga hardware models supported by the Quick Start screen.
 * cmdArg values match the --model handler in main.cpp (lines 1171-1217).
 */
enum class AmigaModel(
	val cmdArg: String,
	val displayName: String,
	val description: String,
	val hasFloppy: Boolean,
	val hasCd: Boolean,
	val chipset: String,
	val defaultCpu: String
) {
	A500(
		cmdArg = "A500",
		displayName = "Amiga 500",
		description = "OCS, 68000, 512KB Chip + 512KB Slow",
		hasFloppy = true,
		hasCd = false,
		chipset = "OCS",
		defaultCpu = "68000"
	),
	A500_PLUS(
		cmdArg = "A500P",
		displayName = "Amiga 500+",
		description = "ECS, 68000, 1MB Chip",
		hasFloppy = true,
		hasCd = false,
		chipset = "ECS",
		defaultCpu = "68000"
	),
	A600(
		cmdArg = "A600",
		displayName = "Amiga 600",
		description = "ECS, 68000, 2MB Chip",
		hasFloppy = true,
		hasCd = false,
		chipset = "ECS",
		defaultCpu = "68000"
	),
	A1000(
		cmdArg = "A1000",
		displayName = "Amiga 1000",
		description = "OCS, 68000, 512KB Chip",
		hasFloppy = true,
		hasCd = false,
		chipset = "OCS",
		defaultCpu = "68000"
	),
	A2000(
		cmdArg = "A2000",
		displayName = "Amiga 2000",
		description = "OCS/ECS, 68000, 512KB Chip + 512KB Slow",
		hasFloppy = true,
		hasCd = false,
		chipset = "OCS",
		defaultCpu = "68000"
	),
	A1200(
		cmdArg = "A1200",
		displayName = "Amiga 1200",
		description = "AGA, 68020, 2MB Chip",
		hasFloppy = true,
		hasCd = false,
		chipset = "AGA",
		defaultCpu = "68020"
	),
	A3000(
		cmdArg = "A3000",
		displayName = "Amiga 3000",
		description = "ECS, 68030, 2MB Chip + 8MB Fast",
		hasFloppy = true,
		hasCd = false,
		chipset = "ECS",
		defaultCpu = "68030"
	),
	A4000(
		cmdArg = "A4000",
		displayName = "Amiga 4000",
		description = "AGA, 68040, 2MB Chip + 8MB Fast",
		hasFloppy = true,
		hasCd = false,
		chipset = "AGA",
		defaultCpu = "68040"
	),
	CD32(
		cmdArg = "CD32",
		displayName = "CD32",
		description = "AGA, 68020, 2MB Chip, CD-ROM",
		hasFloppy = false,
		hasCd = true,
		chipset = "AGA",
		defaultCpu = "68020"
	),
	CDTV(
		cmdArg = "CDTV",
		displayName = "CDTV",
		description = "OCS, 68000, 1MB Chip, CD-ROM",
		hasFloppy = false,
		hasCd = true,
		chipset = "OCS",
		defaultCpu = "68000"
	)
}
