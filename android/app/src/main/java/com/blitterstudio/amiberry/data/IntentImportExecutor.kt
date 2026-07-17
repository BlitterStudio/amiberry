package com.blitterstudio.amiberry.data

import android.content.Context
import android.net.Uri
import android.util.Log
import com.blitterstudio.amiberry.data.model.AmigaModel
import com.blitterstudio.amiberry.data.model.EmulatorSettings
import com.blitterstudio.amiberry.data.model.FileCategory
import com.blitterstudio.amiberry.data.model.StoragePaths
import java.io.File

object IntentImportExecutor {

	data class PreparedImport(
		val feedback: ImportFeedback.Message,
		val launch: Launch? = null
	)

	sealed interface Launch {
		data class SavedConfig(
			val configPath: String,
			val controlSettings: EmulatorSettings
		) : Launch

		data class QuickStart(
			val model: AmigaModel,
			val floppyPath: String?,
			val floppy1Path: String?,
			val cdPath: String?,
			val configPath: String
		) : Launch

		data class WhdLoad(
			val lhaPath: String,
			val configPath: String
		) : Launch

		data class Rp9(val path: String) : Launch
	}

	suspend fun importAndPrepare(context: Context, uri: Uri): PreparedImport {
		val rawName = FileManager.getDisplayName(context, uri) ?: uri.lastPathSegment
		return when (val import = IntentImport.classify(rawName)) {
			is IntentImport.Classification.Config -> importConfig(context, uri, import.safeName)
			is IntentImport.Classification.Rp9 -> importRp9(context, uri, import.safeName)
			is IntentImport.Classification.Media -> importMedia(context, uri, import)
			is IntentImport.Classification.Unsupported -> {
				Log.w(TAG, "Ignoring unsupported file from intent: ${import.safeName}")
				PreparedImport(ImportFeedback.unsupportedFileType(import.extension))
			}
		}
	}

	internal fun mediaLaunchFor(
		category: FileCategory,
		importedPath: String,
		configPath: String
	): Launch? =
		when (category) {
			FileCategory.WHDLOAD_GAMES -> Launch.WhdLoad(
				lhaPath = importedPath,
				configPath = configPath
			)
			FileCategory.FLOPPIES -> Launch.QuickStart(
				model = AmigaModel.A500,
				floppyPath = importedPath,
				floppy1Path = null,
				cdPath = null,
				configPath = configPath
			)
			FileCategory.CD_IMAGES -> Launch.QuickStart(
				model = AmigaModel.CD32,
				floppyPath = null,
				floppy1Path = null,
				cdPath = importedPath,
				configPath = configPath
			)
			FileCategory.ROMS,
			FileCategory.HARD_DRIVES -> null
		}

	private fun importConfig(context: Context, uri: Uri, safeName: String): PreparedImport {
		val baseDir = context.getExternalFilesDir(null)
			?: return PreparedImport(ImportFeedback.importFailed(safeName))
		val confDir = File(baseDir, StoragePaths.CONFIGURATIONS)
		if (!confDir.exists() && !confDir.mkdirs()) {
			Log.e(TAG, "Failed to create config directory: ${confDir.absolutePath}")
			return PreparedImport(ImportFeedback.importFailed(safeName))
		}
		val targetFile = FileManager.uniqueImportTarget(confDir, safeName)

		return try {
			val inputStream = context.contentResolver.openInputStream(uri)
			if (inputStream == null) {
				Log.e(TAG, "Failed to open input stream for config URI: $uri")
				return PreparedImport(ImportFeedback.importFailed(safeName))
			}
			inputStream.use { input ->
				targetFile.outputStream().use { output -> input.copyTo(output) }
			}
			PreparedImport(
				feedback = ImportFeedback.configImported(targetFile.name),
				launch = Launch.SavedConfig(
					configPath = targetFile.absolutePath,
					controlSettings = AndroidLaunchConfig.loadLastSessionSettings(context)
				)
			)
		} catch (e: Exception) {
			Log.e(TAG, "Failed to import config from intent", e)
			PreparedImport(ImportFeedback.importFailed(safeName))
		}
	}

	private fun importRp9(context: Context, uri: Uri, safeName: String): PreparedImport {
		val baseDir = context.getExternalFilesDir(null)
			?: return PreparedImport(ImportFeedback.importFailed(safeName))
		val rp9Dir = File(baseDir, StoragePaths.RP9)
		if (!rp9Dir.exists() && !rp9Dir.mkdirs())
			return PreparedImport(ImportFeedback.importFailed(safeName))
		val targetFile = FileManager.uniqueImportTarget(rp9Dir, safeName)
		return try {
			val inputStream = context.contentResolver.openInputStream(uri)
				?: return PreparedImport(ImportFeedback.importFailed(safeName))
			inputStream.use { input -> targetFile.outputStream().use { output -> input.copyTo(output) } }
			PreparedImport(ImportFeedback.fileImported(targetFile.name), Launch.Rp9(targetFile.absolutePath))
		} catch (e: Exception) {
			Log.e(TAG, "Failed to import RP9 package", e)
			PreparedImport(ImportFeedback.importFailed(safeName))
		}
	}

	private suspend fun importMedia(
		context: Context,
		uri: Uri,
		import: IntentImport.Classification.Media
	): PreparedImport {
		return when (val result = FileManager.importFileWithResult(context, uri, import.category)) {
			is FileManager.ImportResult.Imported -> prepareImportedMedia(context, result.path, result.safeName, import.category)
			is FileManager.ImportResult.Unsupported -> PreparedImport(
				ImportFeedback.unsupportedFileType(result.extension)
			)
			is FileManager.ImportResult.Failed -> PreparedImport(
				ImportFeedback.importFailed(result.safeName)
			)
		}
	}

	private suspend fun prepareImportedMedia(
		context: Context,
		importedPath: String,
		importedName: String,
		category: FileCategory
	): PreparedImport {
		return try {
			FileRepository.getInstance(context).rescanCategory(category)
			val feedback = ImportFeedback.fileImported(importedName)
			val currentSettings = AndroidLaunchConfig.loadLastSessionSettings(context)
			val configPath = when (category) {
				FileCategory.WHDLOAD_GAMES -> AndroidLaunchConfig.writeControlConfig(context, currentSettings).absolutePath
				FileCategory.FLOPPIES -> writeIntentQuickStartConfig(
					context = context,
					model = AmigaModel.A500,
					currentSettings = currentSettings,
					floppy0 = importedPath,
					floppy1 = "",
					cdImage = ""
				)
				FileCategory.CD_IMAGES -> writeIntentQuickStartConfig(
					context = context,
					model = AmigaModel.CD32,
					currentSettings = currentSettings,
					floppy0 = "",
					floppy1 = "",
					cdImage = importedPath
				)
				FileCategory.ROMS,
				FileCategory.HARD_DRIVES -> null
			}
			PreparedImport(
				feedback = feedback,
				launch = configPath?.let { mediaLaunchFor(category, importedPath, it) }
			)
		} catch (e: Exception) {
			Log.e(TAG, "Failed to prepare imported media from intent", e)
			PreparedImport(ImportFeedback.importFailed(importedName))
		}
	}

	private fun writeIntentQuickStartConfig(
		context: Context,
		model: AmigaModel,
		currentSettings: EmulatorSettings,
		floppy0: String,
		floppy1: String,
		cdImage: String
	): String {
		val roms = FileManager.scanForCategory(context, FileCategory.ROMS)
		val launchSettings = AndroidLaunchConfig.buildQuickStartSettingsSnapshot(
			model = model,
			roms = roms,
			currentSettings = currentSettings,
			floppy0 = floppy0,
			floppy1 = floppy1,
			cdImage = cdImage
		)
		return AndroidLaunchConfig.writeQuickStartConfig(context, launchSettings).absolutePath
	}

	private const val TAG = "Amiberry-IntentImport"
}
