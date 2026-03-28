package com.blitterstudio.amiberry.ui.screens

import android.content.Intent
import android.net.Uri
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
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AboutScreen(onBack: () -> Unit) {
	val context = LocalContext.current
	val versionName = try {
		context.packageManager.getPackageInfo(context.packageName, 0).versionName ?: "unknown"
	} catch (_: Exception) {
		"unknown"
	}

	Scaffold(
		topBar = {
			TopAppBar(
				title = { Text(stringResource(R.string.about_title)) },
				navigationIcon = {
					IconButton(onClick = onBack) {
						Icon(
							Icons.AutoMirrored.Filled.ArrowBack,
							contentDescription = null
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
					LinkRow(
						icon = Icons.Default.Code,
						text = stringResource(R.string.about_link_github),
						onClick = {
							context.startActivity(Intent(Intent.ACTION_VIEW, Uri.parse("https://github.com/BlitterStudio/amiberry")))
						}
					)
					LinkRow(
						icon = Icons.AutoMirrored.Filled.MenuBook,
						text = stringResource(R.string.about_link_wiki),
						onClick = {
							context.startActivity(Intent(Intent.ACTION_VIEW, Uri.parse("https://github.com/BlitterStudio/amiberry/wiki")))
						}
					)
					LinkRow(
						icon = Icons.AutoMirrored.Filled.Chat,
						text = stringResource(R.string.about_link_discord),
						onClick = {
							context.startActivity(Intent(Intent.ACTION_VIEW, Uri.parse("https://discord.gg/wWndKTGpGV")))
						}
					)
					LinkRow(
						icon = Icons.Default.Shield,
						text = stringResource(R.string.about_link_privacy),
						onClick = {
							context.startActivity(Intent(Intent.ACTION_VIEW, Uri.parse("https://amiberry.com/privacy-policy")))
						}
					)
				}
			}

			// Credits
			OutlinedCard(modifier = Modifier.fillMaxWidth()) {
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
			OutlinedCard(modifier = Modifier.fillMaxWidth()) {
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

@Composable
private fun LinkRow(icon: ImageVector, text: String, onClick: () -> Unit) {
	TextButton(
		onClick = onClick,
		modifier = Modifier.fillMaxWidth()
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
