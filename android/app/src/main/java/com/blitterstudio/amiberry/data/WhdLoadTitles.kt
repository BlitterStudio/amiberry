package com.blitterstudio.amiberry.data

/**
 * Readable titles for WHDLoad archives, whose file names run the words together and
 * carry the installer's tag: "TheLostPixelv1.3_WHD4.lha" reads "The Lost Pixel v1.3",
 * "CastlevaniaAGA_WHDLoad.lha" reads "Castlevania AGA".
 */
object WhdLoadTitles {
	private val ARCHIVE_SUFFIXES = listOf(".lha", ".lzh", ".lzx")

	/** A trailing installer tag: "_WHDLoad", "_WHD", "-WHD4". */
	private val WHD_TAG = Regex("[ _-]*WHD(?:Load)?\\d*$", RegexOption.IGNORE_CASE)

	private val SPACES = Regex(" {2,}")

	fun clean(fileName: String): String {
		val base = ARCHIVE_SUFFIXES.fold(fileName) { name, suffix ->
			if (name.endsWith(suffix, ignoreCase = true)) name.dropLast(suffix.length) else name
		}
		val name = base.replace(WHD_TAG, "").replace('_', ' ')
		val spaced = StringBuilder(name.length + 8)
		name.forEachIndexed { index, c ->
			if (index > 0 && c != ' ' && spaced.last() != ' ' && startsWord(name, index)) spaced.append(' ')
			spaced.append(c)
		}
		return spaced.toString().replace(SPACES, " ").trim().ifEmpty { base.ifEmpty { fileName } }
	}

	private fun startsWord(name: String, index: Int): Boolean {
		val previous = name[index - 1]
		val c = name[index]
		val next = name.getOrNull(index + 1)
		return when {
			// "LostPixel", "CastlevaniaAGA"
			previous.isLowerCase() && c.isUpperCase() -> true
			// "AGAGame": the last capital of a run starts the next word.
			previous.isUpperCase() && c.isUpperCase() && next?.isLowerCase() == true -> true
			// A version after a word: "Pixelv1.3"
			previous.isLetter() && c == 'v' && next?.isDigit() == true -> true
			// A number after a word: "Lemmings2" (not "A1200" or "v1.3")
			previous.isLowerCase() && previous != 'v' && c.isDigit() -> true
			else -> false
		}
	}
}
