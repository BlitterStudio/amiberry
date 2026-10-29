package com.blitterstudio.amiberry.ui.screens

import android.content.ActivityNotFoundException
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import android.content.pm.ApplicationInfo
import android.net.Uri
import android.os.Build
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.automirrored.filled.Chat
import androidx.compose.material.icons.automirrored.filled.MenuBook
import androidx.compose.material.icons.filled.Code
import androidx.compose.material.icons.filled.Shield
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.AboutActions
import com.blitterstudio.amiberry.data.AboutSupportInfo
import com.blitterstudio.amiberry.data.ConfigurationActions
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.dpadFocusable
import com.blitterstudio.amiberry.ui.hasFullStorageAccess
import com.blitterstudio.amiberry.ui.isTvDevice
import androidx.compose.material.icons.filled.ContentCopy
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import kotlinx.coroutines.launch

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AboutScreen(onBack: () -> Unit) {
	val context = LocalContext.current
	val scope = rememberCoroutineScope()
	val snackbarHostState = remember { SnackbarHostState() }
	val tv = isTvDevice()
	// On a TV, focus starts on Back, so OK returns where About was opened from.
	val backFocus = remember { FocusRequester() }
	if (tv) LaunchedEffect(Unit) { backFocus.requestFocus() }
	val packageInfo = try {
		context.packageManager.getPackageInfo(context.packageName, 0)
	} catch (_: Exception) {
		null
	}
	val versionName = packageInfo?.versionName ?: "unknown"
	val versionCode = packageInfo?.longVersionCode ?: 0L
	val buildType = if ((context.applicationInfo.flags and ApplicationInfo.FLAG_DEBUGGABLE) != 0) {
		"debug"
	} else {
		"release"
	}
	val supportInfo = AboutSupportInfo.format(
		versionName = versionName,
		versionCode = versionCode,
		packageName = context.packageName,
		buildType = buildType,
		androidRelease = Build.VERSION.RELEASE.orEmpty(),
		sdkInt = Build.VERSION.SDK_INT,
		primaryAbi = Build.SUPPORTED_ABIS.firstOrNull().orEmpty(),
		device = listOf(Build.MANUFACTURER, Build.MODEL)
			.filter { it.isNotBlank() }
			.joinToString(separator = " "),
		hasFullStorageAccess = hasFullStorageAccess(context),
		externalFilesDir = context.getExternalFilesDir(null)?.absolutePath
	)
	fun actionMessage(message: ConfigurationActions.Message): String =
		message.argument?.let { context.getString(message.stringRes, it) }
			?: context.getString(message.stringRes)
	val linkOpenFailedMessage = actionMessage(AboutActions.openLinkFailedMessage())
	val supportInfoCopiedMessage = actionMessage(AboutActions.supportInfoCopiedMessage())

	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		topBar = {
			TopAppBar(
				title = { Text(stringResource(R.string.about_title)) },
				navigationIcon = {
					IconButton(
						onClick = onBack,
						modifier = Modifier.focusRequester(backFocus).dpadFocusIndicator()
					) {
						Icon(
							Icons.AutoMirrored.Filled.ArrowBack,
							contentDescription = stringResource(R.string.action_back)
						)
					}
				}
			)
		}
	) { innerPadding ->
		Column(
			modifier = Modifier
				.fillMaxSize()
				.padding(innerPadding)
				.verticalScroll(rememberScrollState())
				.padding(16.dp),
			verticalArrangement = Arrangement.spacedBy(16.dp)
		) {
			// App identity + version
			Card(
				modifier = Modifier.fillMaxWidth(),
				colors = CardDefaults.cardColors(
					containerColor = MaterialTheme.colorScheme.primaryContainer
				)
			) {
				Column(modifier = Modifier.padding(16.dp)) {
					Text(
						stringResource(R.string.app_name),
						style = MaterialTheme.typography.headlineMedium,
						color = MaterialTheme.colorScheme.onPrimaryContainer
					)
					Spacer(modifier = Modifier.height(4.dp))
					Text(
						text = "${stringResource(R.string.about_version_label)} $versionName",
						style = MaterialTheme.typography.bodyLarge,
						color = MaterialTheme.colorScheme.onPrimaryContainer
					)
					Spacer(modifier = Modifier.height(12.dp))
					Text(
						text = stringResource(R.string.about_description),
						style = MaterialTheme.typography.bodyMedium,
						color = MaterialTheme.colorScheme.onPrimaryContainer
					)
				}
			}

			// Links
			OutlinedCard(modifier = Modifier.fillMaxWidth()) {
				Column(modifier = Modifier.padding(16.dp)) {
					Text(
						stringResource(R.string.about_links_title),
						style = MaterialTheme.typography.titleMedium
					)
					Spacer(modifier = Modifier.height(8.dp))
					AboutActions.externalLinks().forEach { link ->
						if (tv) {
							// A TV has no browser to open them: show where they lead instead.
							LinkText(icon = iconForLink(link.id), text = stringResource(link.labelRes), url = link.url)
						} else {
							LinkRow(
								icon = iconForLink(link.id),
								text = stringResource(link.labelRes),
								onClick = {
									openExternalLink(context, link.url) {
										scope.launch { snackbarHostState.showSnackbar(linkOpenFailedMessage) }
									}
								}
							)
						}
					}
				}
			}

			// Support info
			OutlinedCard(modifier = Modifier.fillMaxWidth()) {
				Column(modifier = Modifier.padding(16.dp)) {
					// Copy sits beside the title, in view before the (long) info itself.
					Row(
						modifier = Modifier.fillMaxWidth(),
						verticalAlignment = Alignment.CenterVertically
					) {
						Text(
							stringResource(R.string.about_support_title),
							style = MaterialTheme.typography.titleMedium,
							modifier = Modifier.weight(1f)
						)
						TextButton(
							onClick = {
								val clipboard = context.getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
								clipboard.setPrimaryClip(
									ClipData.newPlainText(context.getString(R.string.about_support_title), supportInfo)
								)
								scope.launch { snackbarHostState.showSnackbar(supportInfoCopiedMessage) }
							},
							modifier = Modifier.dpadFocusIndicator()
						) {
							Icon(Icons.Default.ContentCopy, contentDescription = null, modifier = Modifier.size(18.dp))
							Spacer(modifier = Modifier.width(6.dp))
							Text(stringResource(R.string.about_copy_support_info))
						}
					}
					Spacer(modifier = Modifier.height(8.dp))
					Text(
						text = supportInfo,
						style = MaterialTheme.typography.bodyMedium,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
				}
			}

			// Credits and license have no control: on a TV they take focus so the D-pad reaches them.
			val readOnTv = if (tv) Modifier.dpadFocusable() else Modifier
			OutlinedCard(modifier = Modifier.fillMaxWidth().then(readOnTv)) {
				Column(modifier = Modifier.padding(16.dp)) {
					Text(
						stringResource(R.string.about_credits_title),
						style = MaterialTheme.typography.titleMedium
					)
					Spacer(modifier = Modifier.height(8.dp))
					Text(
						text = stringResource(R.string.about_credits_body),
						style = MaterialTheme.typography.bodyMedium,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
				}
			}

			// License
			OutlinedCard(modifier = Modifier.fillMaxWidth().then(readOnTv)) {
				Column(modifier = Modifier.padding(16.dp)) {
					Text(
						stringResource(R.string.about_license_title),
						style = MaterialTheme.typography.titleMedium
					)
					Spacer(modifier = Modifier.height(8.dp))
					Text(
						text = stringResource(R.string.about_license_body),
						style = MaterialTheme.typography.bodyMedium,
						color = MaterialTheme.colorScheme.onSurfaceVariant
					)
				}
			}

			Spacer(modifier = Modifier.height(16.dp))
		}
	}
}

private fun iconForLink(id: AboutActions.LinkId): ImageVector =
	when (id) {
		AboutActions.LinkId.SourceCode -> Icons.Default.Code
		AboutActions.LinkId.Wiki -> Icons.AutoMirrored.Filled.MenuBook
		AboutActions.LinkId.Discord -> Icons.AutoMirrored.Filled.Chat
		AboutActions.LinkId.Privacy -> Icons.Default.Shield
	}

private fun openExternalLink(context: Context, url: String, onFailure: () -> Unit) {
	try {
		context.startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(url)))
	} catch (_: ActivityNotFoundException) {
		onFailure()
	} catch (_: SecurityException) {
		onFailure()
	}
}

@Composable
private fun LinkRow(icon: ImageVector, text: String, onClick: () -> Unit) {
	TextButton(
		onClick = onClick,
		modifier = Modifier.fillMaxWidth().dpadFocusIndicator()
	) {
		Row(
			modifier = Modifier.fillMaxWidth(),
			verticalAlignment = Alignment.CenterVertically
		) {
			Icon(
				icon,
				contentDescription = null,
				modifier = Modifier.size(20.dp),
				tint = MaterialTheme.colorScheme.primary
			)
			Spacer(modifier = Modifier.width(12.dp))
			Text(text, style = MaterialTheme.typography.bodyLarge)
		}
	}
}

/** A link as text, for a TV: focusable so the D-pad scrolls through the card. */
@Composable
private fun LinkText(icon: ImageVector, text: String, url: String) {
	Row(
		modifier = Modifier
			.fillMaxWidth()
			.dpadFocusable()
			.padding(horizontal = 12.dp, vertical = 8.dp),
		verticalAlignment = Alignment.CenterVertically
	) {
		Icon(
			icon,
			contentDescription = null,
			modifier = Modifier.size(20.dp),
			tint = MaterialTheme.colorScheme.primary
		)
		Spacer(modifier = Modifier.width(12.dp))
		Column {
			Text(text, style = MaterialTheme.typography.bodyLarge)
			Text(url, style = MaterialTheme.typography.bodyMedium, color = MaterialTheme.colorScheme.onSurfaceVariant)
		}
	}
}
