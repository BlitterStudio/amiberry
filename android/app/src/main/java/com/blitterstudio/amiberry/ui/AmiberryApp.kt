package com.blitterstudio.amiberry.ui

import android.content.res.Configuration
import android.widget.Toast
import androidx.activity.compose.BackHandler
import androidx.compose.foundation.focusGroup
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Icon
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationRail
import androidx.compose.material3.NavigationRailItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.ui.findActivity
import androidx.navigation.NavDestination.Companion.hierarchy
import androidx.navigation.NavGraph.Companion.findStartDestination
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import com.blitterstudio.amiberry.data.GlobalDialogState
import com.blitterstudio.amiberry.data.PendingImportState
import com.blitterstudio.amiberry.ui.navigation.Screen
import com.blitterstudio.amiberry.ui.navigation.shouldMoveTaskToBack
import com.blitterstudio.amiberry.ui.screens.AboutScreen
import com.blitterstudio.amiberry.ui.screens.ConfigurationsScreen
import com.blitterstudio.amiberry.ui.screens.FileManagerScreen
import com.blitterstudio.amiberry.ui.screens.QuickStartScreen
import com.blitterstudio.amiberry.ui.screens.settings.SettingsScreen
import androidx.activity.ComponentActivity
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.rememberCoroutineScope
import androidx.lifecycle.viewmodel.compose.viewModel
import com.blitterstudio.amiberry.data.ConfigurationSaveActions
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel
import kotlinx.coroutines.launch

@Composable
fun AmiberryApp() {
	val navController = rememberNavController()
	val context = LocalContext.current
	val activity = context.findActivity() as? MainActivity
	val crashDetected = activity?.emulatorCrashDetected == true
	val assetExtractionFailed = activity?.assetExtractionFailed == true
	val visibleGlobalDialog = GlobalDialogState.visibleDialog(
		crashDetected = crashDetected,
		assetExtractionFailed = assetExtractionFailed
	)

	when (visibleGlobalDialog) {
		GlobalDialogState.VisibleDialog.AssetExtractionFailure -> activity?.let { activity ->
			val assetExtractionDetails = activity.assetExtractionFailureDetails()
			val assetExtractionInProgress = activity.assetExtractionInProgress == true
			AlertDialog(
				onDismissRequest = {},
				title = { Text(stringResource(R.string.asset_extraction_failed_title)) },
				text = {
					Column {
						Text(stringResource(R.string.asset_extraction_failed_message))
						if (assetExtractionDetails != null) {
							Spacer(modifier = Modifier.height(8.dp))
							Text(stringResource(R.string.asset_extraction_failure_summary, assetExtractionDetails))
						}
					}
				},
				dismissButton = {
					TextButton(onClick = { activity.copyAssetExtractionFailureDetailsToClipboard() }) {
						Text(stringResource(R.string.asset_extraction_copy_details))
					}
				},
				confirmButton = {
					TextButton(onClick = { activity.retryAssetExtraction() }, enabled = !assetExtractionInProgress) {
						Text(
							stringResource(
								if (assetExtractionInProgress) R.string.action_retrying else R.string.action_retry
							)
						)
					}
				}
			)
		}
		GlobalDialogState.VisibleDialog.CrashRecovery -> activity?.let { activity ->
			val crashSummary = activity.crashDiagnosticsSummary()
			AlertDialog(
				onDismissRequest = { activity.clearCrashFlag() },
				title = { Text(stringResource(R.string.crash_dialog_title)) },
				text = {
					Column {
						Text(stringResource(R.string.crash_dialog_message))
						if (crashSummary != null) {
							Spacer(modifier = Modifier.height(8.dp))
							Text(stringResource(R.string.crash_dialog_diagnostics_summary, crashSummary))
						}
					}
				},
				dismissButton = {
					TextButton(onClick = { activity.copyCrashDiagnosticsToClipboard() }) {
						Text(stringResource(R.string.crash_dialog_copy_diagnostics))
					}
				},
				confirmButton = {
					TextButton(onClick = { activity.clearCrashFlag() }) {
						Text(stringResource(R.string.crash_dialog_dismiss))
					}
				}
			)
		}
		GlobalDialogState.VisibleDialog.None -> Unit
	}

	// Handle pending file from ACTION_VIEW intent
	val pendingUri = activity?.pendingFileUri
	val pendingMimeType = activity?.pendingFileMimeType
	val assetsReady = activity?.isReady == true
	val importLaunchInProgress = activity?.importLaunchInProgress == true
	LaunchedEffect(pendingUri, pendingMimeType, assetsReady, crashDetected, assetExtractionFailed, importLaunchInProgress) {
		val mainActivity = activity ?: return@LaunchedEffect
		val uri = pendingUri ?: return@LaunchedEffect
		if (PendingImportState.shouldProcess(
				hasPendingUri = true,
				assetsReady = assetsReady,
				assetExtractionFailed = assetExtractionFailed,
				crashRecoveryVisible = crashDetected,
				importLaunchInProgress = importLaunchInProgress
			)
		) {
			mainActivity.clearPendingFileUri()
			mainActivity.importAndLaunch(uri, pendingMimeType)
		}
	}

	val configuration = LocalConfiguration.current
	val useNavRail = configuration.orientation == Configuration.ORIENTATION_LANDSCAPE

	val navBackStackEntry by navController.currentBackStackEntryAsState()
	val currentDestination = navBackStackEntry?.destination

	val settingsViewModel: SettingsViewModel =
		viewModel(LocalContext.current.findActivity() as ComponentActivity)
	val navScope = rememberCoroutineScope()
	var pendingNavigation by remember { mutableStateOf<(() -> Unit)?>(null) }

	fun navigateToRoute(route: String) {
		navController.navigate(route) {
			popUpTo(navController.graph.findStartDestination().id) { saveState = true }
			launchSingleTop = true
			restoreState = true
		}
	}

	val onSettingsRoute = currentDestination?.hierarchy?.any { it.route == Screen.Settings.route } == true

	fun guardedNavigate(targetRoute: String, action: () -> Unit) {
		if (targetRoute != Screen.Settings.route && onSettingsRoute && settingsViewModel.isDirty) {
			pendingNavigation = action
		} else {
			action()
		}
	}

	BackHandler(
		enabled = shouldMoveTaskToBack(
			route = currentDestination?.route,
			crashDialogVisible = crashDetected,
			assetExtractionFailed = assetExtractionFailed
		)
	) {
		activity?.moveTaskToBack(true)
	}

	BackHandler(enabled = onSettingsRoute && settingsViewModel.isDirty) {
		pendingNavigation = { activity?.moveTaskToBack(true) }
	}

	pendingNavigation?.let { proceed ->
		val configName = settingsViewModel.currentConfigName.orEmpty()
		AlertDialog(
			onDismissRequest = { pendingNavigation = null },
			title = { Text(stringResource(R.string.dialog_unsaved_changes_title)) },
			text = { Text(stringResource(R.string.dialog_unsaved_changes_message, configName)) },
			confirmButton = {
				TextButton(onClick = {
					navScope.launch {
						val result = settingsViewModel.saveTracked()
						if (result is ConfigurationSaveActions.SaveResult.Saved) {
							pendingNavigation = null
							proceed()
						} else {
							pendingNavigation = null
							Toast.makeText(
								context,
								context.getString(R.string.msg_failed_save_config),
								Toast.LENGTH_SHORT
							).show()
						}
					}
				}) {
					Text(stringResource(R.string.action_save))
				}
			},
			dismissButton = {
				Row {
					TextButton(onClick = {
						settingsViewModel.discardChanges()
						pendingNavigation = null
						proceed()
					}) {
						Text(stringResource(R.string.action_discard))
					}
					TextButton(onClick = { pendingNavigation = null }) {
						Text(stringResource(R.string.action_cancel))
					}
				}
			}
		)
	}

	if (useNavRail) {
		Row(modifier = Modifier.fillMaxSize()) {
			NavigationRail(modifier = Modifier.focusGroup()) {
					Screen.bottomNavItems.forEach { screen ->
						val title = stringResource(screen.titleRes)
						NavigationRailItem(
							icon = { Icon(screen.icon, contentDescription = title) },
							label = { Text(title) },
							selected = currentDestination?.hierarchy?.any { it.route == screen.route } == true,
						onClick = { guardedNavigate(screen.route) { navigateToRoute(screen.route) } }
					)
				}
			}
			Scaffold(modifier = Modifier.weight(1f)) { innerPadding ->
				AmiberryNavHost(navController, Modifier.padding(innerPadding))
			}
		}
	} else {
		Scaffold(
			bottomBar = {
				NavigationBar(modifier = Modifier.focusGroup()) {
					Screen.bottomNavItems.forEach { screen ->
						val title = stringResource(screen.titleRes)
						NavigationBarItem(
							icon = { Icon(screen.icon, contentDescription = title) },
							label = { Text(title) },
							selected = currentDestination?.hierarchy?.any { it.route == screen.route } == true,
							onClick = { guardedNavigate(screen.route) { navigateToRoute(screen.route) } }
						)
					}
				}
			}
		) { innerPadding ->
			AmiberryNavHost(navController, Modifier.padding(innerPadding))
		}
	}
}

@Composable
private fun AmiberryNavHost(navController: NavHostController, modifier: Modifier = Modifier) {
	NavHost(
		navController = navController,
		startDestination = Screen.QuickStart.route,
		modifier = modifier
	) {
		composable(Screen.QuickStart.route) {
			QuickStartScreen(navController = navController)
		}
		composable(Screen.About.route) {
			AboutScreen(onBack = { navController.popBackStack() })
		}
		composable(Screen.Settings.route) {
			SettingsScreen()
		}
		composable(Screen.FileManager.route) {
			FileManagerScreen()
		}
		composable(Screen.Configurations.route) {
			ConfigurationsScreen(navController = navController)
		}
	}
}
