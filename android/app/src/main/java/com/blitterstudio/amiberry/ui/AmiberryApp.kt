package com.blitterstudio.amiberry.ui

import android.content.res.Configuration
import androidx.compose.foundation.focusGroup
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
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
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.ui.findActivity
import androidx.navigation.NavDestination.Companion.hierarchy
import androidx.navigation.NavGraph.Companion.findStartDestination
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import com.blitterstudio.amiberry.ui.navigation.Screen
import com.blitterstudio.amiberry.ui.screens.AboutScreen
import com.blitterstudio.amiberry.ui.screens.ConfigurationsScreen
import com.blitterstudio.amiberry.ui.screens.FileManagerScreen
import com.blitterstudio.amiberry.ui.screens.QuickStartScreen
import com.blitterstudio.amiberry.ui.screens.settings.SettingsScreen

@Composable
fun AmiberryApp() {
	val navController = rememberNavController()
	val activity = LocalContext.current.findActivity() as? MainActivity

	// Show crash recovery dialog when emulator process died unexpectedly
	if (activity?.emulatorCrashDetected == true) {
		AlertDialog(
			onDismissRequest = { activity.clearCrashFlag() },
			title = { Text(stringResource(R.string.crash_dialog_title)) },
			text = { Text(stringResource(R.string.crash_dialog_message)) },
			confirmButton = {
				TextButton(onClick = { activity.clearCrashFlag() }) {
					Text(stringResource(R.string.crash_dialog_dismiss))
				}
			}
		)
	}

	val configuration = LocalConfiguration.current
	val useNavRail = configuration.orientation == Configuration.ORIENTATION_LANDSCAPE

	val navBackStackEntry by navController.currentBackStackEntryAsState()
	val currentDestination = navBackStackEntry?.destination

	if (useNavRail) {
		Row(modifier = Modifier.fillMaxSize()) {
			NavigationRail(modifier = Modifier.focusGroup()) {
					Screen.bottomNavItems.forEach { screen ->
						val title = stringResource(screen.titleRes)
						NavigationRailItem(
							icon = { Icon(screen.icon, contentDescription = title) },
							label = { Text(title) },
							selected = currentDestination?.hierarchy?.any { it.route == screen.route } == true,
						onClick = {
							navController.navigate(screen.route) {
								popUpTo(navController.graph.findStartDestination().id) {
									saveState = true
								}
								launchSingleTop = true
								restoreState = true
							}
						}
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
							onClick = {
								navController.navigate(screen.route) {
									popUpTo(navController.graph.findStartDestination().id) {
										saveState = true
									}
									launchSingleTop = true
									restoreState = true
								}
							}
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
