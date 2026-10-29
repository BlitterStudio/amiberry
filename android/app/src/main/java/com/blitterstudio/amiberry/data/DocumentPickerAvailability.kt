package com.blitterstudio.amiberry.data

import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager

/**
 * Whether the system document picker can open files or folders. Google TV resolves
 * ACTION_OPEN_DOCUMENT to a stub that only says "You don't have an app that can do this",
 * so the launcher offers its own browser of the device's storage there instead.
 */
object DocumentPickerAvailability {

	private const val TV_FRAMEWORK_STUBS_PACKAGE = "com.android.tv.frameworkpackagestubs"

	fun canOpenDocuments(context: Context): Boolean =
		resolves(context, Intent(Intent.ACTION_OPEN_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE).setType("*/*"))

	fun canOpenDocumentTree(context: Context): Boolean =
		resolves(context, Intent(Intent.ACTION_OPEN_DOCUMENT_TREE))

	/** A picker resolved to [packageName] (null: nothing resolved) can really pick documents. */
	internal fun isUsablePicker(packageName: String?): Boolean =
		packageName != null && packageName != TV_FRAMEWORK_STUBS_PACKAGE

	private fun resolves(context: Context, intent: Intent): Boolean =
		isUsablePicker(
			context.packageManager
				.resolveActivity(intent, PackageManager.MATCH_DEFAULT_ONLY)
				?.activityInfo
				?.packageName
		)
}
