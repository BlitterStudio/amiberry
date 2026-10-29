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
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
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
import androidx.compose.runtime.CompositionLocalProvider

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
					TextButton(
						onClick = { activity.copyAssetExtractionFailureDetailsToClipboard() },
						modifier = Modifier.dpadFocusIndicator()
					) {
						Text(stringResource(R.string.asset_extraction_copy_details))
					}
				},
				confirmButton = {
					TextButton(
						onClick = { activity.retryAssetExtraction() },
						enabled = !assetExtractionInProgress,
						modifier = Modifier.dpadFocusIndicator()
					) {
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
					TextButton(
						onClick = { activity.copyCrashDiagnosticsToClipboard() },
						modifier = Modifier.dpadFocusIndicator()
					) {
						Text(stringResource(R.string.crash_dialog_copy_diagnostics))
					}
				},
				confirmButton = {
					TextButton(
						onClick = { activity.clearCrashFlag() },
						modifier = Modifier.dpadFocusIndicator()
					) {
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
	var pendingGuard by remember { mutableStateOf<UnsavedChangesGuard.Pending?>(null) }

	fun navigateToRoute(route: String) {
		navController.navigate(route) {
			popUpTo(navController.graph.findStartDestination().id) { saveState = true }
			launchSingleTop = true
			restoreState = true
		}
	}

	val onSettingsRoute = currentDestination?.hierarchy?.any { it.route == Screen.Settings.route } == true

	fun guardedNavigate(targetRoute: String, action: () -> Unit) {
		val pending = if (targetRoute != Screen.Settings.route && onSettingsRoute) {
			UnsavedChangesGuard.request(UnsavedChangesGuard.Trigger.Leave, settingsViewModel.isDirty, action)
		} else {
			null
		}
		if (pending != null) pendingGuard = pending else action()
	}

	// Settings' Start asks the same question while a named configuration has unsaved edits.
	val startGuard = remember(settingsViewModel) {
		StartGuard { proceed ->
			val pending = UnsavedChangesGuard.request(
				UnsavedChangesGuard.Trigger.Start,
				settingsViewModel.isDirty,
				proceed
			)
			if (pending != null) pendingGuard = pending else proceed()
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
		pendingGuard = UnsavedChangesGuard.Pending(UnsavedChangesGuard.Trigger.Leave) {
			activity?.moveTaskToBack(true)
		}
	}

	pendingGuard?.let { pending ->
		val configName = settingsViewModel.currentConfigName.orEmpty()
		val starting = pending.trigger == UnsavedChangesGuard.Trigger.Start
		fun answer(choice: UnsavedChangesGuard.Choice) {
			val resolution = UnsavedChangesGuard.resolve(pending.trigger, choice)
			pendingGuard = null
			if (resolution.saveFirst) {
				navScope.launch {
					val result = settingsViewModel.saveTracked()
					if (result is ConfigurationSaveActions.SaveResult.Saved) {
						pending.proceed()
					} else {
						Toast.makeText(
							context,
							context.getString(R.string.msg_failed_save_config),
							Toast.LENGTH_SHORT
						).show()
					}
				}
				return
			}
			if (resolution.discardEdits) settingsViewModel.discardChanges()
			if (resolution.proceed) pending.proceed()
		}
		AlertDialog(
			onDismissRequest = { answer(UnsavedChangesGuard.Choice.Cancel) },
			title = { Text(stringResource(R.string.dialog_unsaved_changes_title)) },
			text = {
				Text(
					stringResource(
						if (starting) R.string.dialog_unsaved_changes_start_message else R.string.dialog_unsaved_changes_message,
						configName
					)
				)
			},
			confirmButton = {
				// Save is the safe default for D-pad focus: an OK pressed at once must not
				// throw the edits away.
				val saveFocus = remember { FocusRequester() }
				LaunchedEffect(Unit) { saveFocus.requestFocus() }
				TextButton(
					onClick = { answer(UnsavedChangesGuard.Choice.Save) },
					modifier = Modifier.focusRequester(saveFocus).dpadFocusIndicator()
				) {
					Text(stringResource(R.string.action_save))
				}
			},
			dismissButton = {
				Row {
					TextButton(
						onClick = { answer(UnsavedChangesGuard.Choice.DontSave) },
						modifier = Modifier.dpadFocusIndicator()
					) {
						// Leaving reverts the edits; starting keeps them open, unsaved.
						Text(stringResource(if (starting) R.string.action_dont_save else R.string.action_discard))
					}
					TextButton(
						onClick = { answer(UnsavedChangesGuard.Choice.Cancel) },
						modifier = Modifier.dpadFocusIndicator()
					) {
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
						onClick = { guardedNavigate(screen.route) { navigateToRoute(screen.route) } },
						modifier = Modifier.dpadFocusIndicator()
					)
				}
			}
			Scaffold(modifier = Modifier.weight(1f)) { innerPadding ->
				AmiberryNavHost(navController, startGuard, Modifier.padding(innerPadding))
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
							onClick = { guardedNavigate(screen.route) { navigateToRoute(screen.route) } },
							modifier = Modifier.dpadFocusIndicator()
						)
					}
				}
			}
		) { innerPadding ->
			AmiberryNavHost(navController, startGuard, Modifier.padding(innerPadding))
		}
	}
}

@Composable
private fun AmiberryNavHost(navController: NavHostController, startGuard: StartGuard, modifier: Modifier = Modifier) {
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
			CompositionLocalProvider(LocalStartGuard provides startGuard) {
				SettingsScreen()
			}
		}
		composable(Screen.FileManager.route) {
			FileManagerScreen()
		}
		composable(Screen.Configurations.route) {
			ConfigurationsScreen(navController = navController)
		}
	}
}
