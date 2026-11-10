package com.blitterstudio.amiberry.ui.screens.appsettings

import android.os.Build
import androidx.activity.compose.BackHandler
import androidx.annotation.StringRes
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
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
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.Search
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuAnchorType
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.VerticalDivider
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.focus.focusRestorer
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.unit.dp
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.lifecycle.viewmodel.compose.viewModel
import androidx.navigation.NavController
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.AppPreferences
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.components.SidePaneEntry
import com.blitterstudio.amiberry.ui.components.SidePaneList
import com.blitterstudio.amiberry.ui.components.settings.SettingSwitchRow
import com.blitterstudio.amiberry.ui.components.settings.dpadMenuItemInitialFocus
import com.blitterstudio.amiberry.ui.components.settings.SettingsSection
import com.blitterstudio.amiberry.ui.contentEntryFocus
import com.blitterstudio.amiberry.ui.dpadArrowsMoveFocus
import com.blitterstudio.amiberry.ui.dpadFieldFocusIndicator
import com.blitterstudio.amiberry.ui.dpadFocusIndicator
import com.blitterstudio.amiberry.ui.dpadVerticalMovesFocus
import com.blitterstudio.amiberry.ui.hasFullStorageAccess
import com.blitterstudio.amiberry.ui.isTvDevice
import com.blitterstudio.amiberry.ui.rememberDpadTypingGate
import com.blitterstudio.amiberry.ui.windowWidthClass
import kotlinx.coroutines.launch
import androidx.compose.runtime.rememberCoroutineScope

/** One page of App Settings: the host settings' groups it holds, in the layer's order. */
private data class AppSettingsPage(
	val id: String,
	@param:StringRes val titleRes: Int,
	val groups: List<String>
)

/** The pages, as the Apple apps' settings group them (U12: General, Defaults for new machines, …). */
private val PAGES = listOf(
	AppSettingsPage("general", R.string.appsettings_group_general, listOf("global_startup", "global_visual")),
	AppSettingsPage("defaults", R.string.appsettings_group_defaults,
		listOf("global_input", "global_touch", "global_display", "global_sound", "global_whdload")),
	AppSettingsPage("folders", R.string.appsettings_group_folders, listOf("global_paths")),
	AppSettingsPage("advanced", R.string.appsettings_group_advanced, listOf("global_advanced"))
)

/** The page About & Licences is, after the groups. */
private const val ABOUT_PAGE = "about"

/** The section title of a host settings' group. */
@StringRes
private fun groupTitle(group: String): Int = when (group) {
	"global_startup" -> R.string.appsettings_section_startup
	"global_input" -> R.string.appsettings_section_input_defaults
	"global_display" -> R.string.appsettings_section_display_defaults
	"global_sound" -> R.string.appsettings_section_sound_defaults
	"global_whdload" -> R.string.appsettings_section_whdload
	"global_touch" -> R.string.appsettings_section_touch_defaults
	"global_visual" -> R.string.appsettings_section_visual
	"global_paths" -> R.string.appsettings_section_paths
	"global_advanced" -> R.string.appsettings_section_advanced
	else -> R.string.appsettings_group_general
}

/**
 * App Settings, under the Library's gear: the emulator's own settings
 * (amiberry.conf) as the schema's host settings describe them, the
 * launcher's app-level settings among them, and About & Licences. Changes
 * apply at once. A phone lists the pages and opens one; a wide window (and
 * a TV) list them beside the page, and the search covers every host
 * setting across the pages.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AppSettingsScreen(
	navController: NavController,
	viewModel: AppSettingsViewModel = viewModel()
) {
	val context = LocalContext.current
	val snackbarHostState = remember { SnackbarHostState() }
	val scope = rememberCoroutineScope()
	val wide = windowWidthClass().showsSideList
	var selectedPage by rememberSaveable { mutableStateOf(PAGES.first().id) }
	// On a phone, whether a page is open over the list of pages; Back returns to the list.
	var pageOpen by rememberSaveable { mutableStateOf(false) }
	var searchOpen by rememberSaveable { mutableStateOf(false) }
	var query by rememberSaveable { mutableStateOf("") }
	val searching = query.isNotBlank()

	// The values as the emulator's own Advanced settings window may have left them: the
	// file is read again on entry and every time the launcher comes back to the front.
	viewModel.reload()
	val lifecycleOwner = LocalLifecycleOwner.current
	DisposableEffect(lifecycleOwner) {
		val observer = LifecycleEventObserver { _, event ->
			if (event == Lifecycle.Event.ON_RESUME) viewModel.reload()
		}
		lifecycleOwner.lifecycle.addObserver(observer)
		onDispose { lifecycleOwner.lifecycle.removeObserver(observer) }
	}

	viewModel.notice?.let { notice ->
		LaunchedEffect(notice) {
			snackbarHostState.showSnackbar(context.getString(notice))
			viewModel.clearNotice()
		}
	}

	fun showMessage(message: String) {
		scope.launch { snackbarHostState.showSnackbar(message) }
	}

	val titles = PAGES.associate { it.id to stringResource(it.titleRes) }
	val aboutTitle = stringResource(R.string.nav_about_licences)
	val entries = PAGES.map { SidePaneEntry(label = titles.getValue(it.id), testTag = "appsettings-page-${it.id}") } +
		SidePaneEntry(label = aboutTitle, testTag = "appsettings-page-about")
	val pageIndex = entries.indexOfFirst { it.testTag == "appsettings-page-$selectedPage" }.coerceAtLeast(0)

	fun pageTitle(): String = when (selectedPage) {
		ABOUT_PAGE -> aboutTitle
		else -> titles[selectedPage] ?: titles.values.first()
	}

	// A page open on a phone: Back returns to the list of pages (search closes first).
	BackHandler(enabled = !wide && pageOpen && !searchOpen) { pageOpen = false }
	BackHandler(enabled = searchOpen) { searchOpen = false; query = "" }

	// Where D-pad focus enters the screen: the page list (phone) or the side list.
	val categoryFocus = remember { FocusRequester() }
	// Where D-pad Right from the list enters the page, restored to the control last focused there.
	val pageContentFocus = remember { FocusRequester() }

	Scaffold(
		snackbarHost = { SnackbarHost(snackbarHostState) },
		topBar = {
			TopAppBar(
				navigationIcon = {
					IconButton(
						onClick = { navController.popBackStack() },
						modifier = Modifier.dpadFocusIndicator()
					) {
						Icon(
							Icons.AutoMirrored.Filled.ArrowBack,
							contentDescription = stringResource(R.string.action_back)
						)
					}
				},
				title = { Text(pageTitle()) },
				actions = {
					IconButton(
						onClick = {
							searchOpen = !searchOpen
							if (!searchOpen) query = ""
						},
						modifier = Modifier.dpadFocusIndicator()
					) {
						Icon(Icons.Default.Search, contentDescription = stringResource(R.string.library_search))
					}
				}
			)
		}
	) { innerPadding ->
		Column(
			modifier = Modifier
				.fillMaxSize()
				.padding(innerPadding)
		) {
			if (searchOpen) {
				AppSettingsSearchRow(
					query = query,
					onQueryChange = { query = it },
					onClose = {
						searchOpen = false
						query = ""
					}
				)
			}
			if (wide) {
				Row(modifier = Modifier.fillMaxSize()) {
					SidePaneList(
						entries = entries,
						selectedIndex = pageIndex,
						onSelect = { index -> selectedPage = pageIdAt(index) },
						modifier = Modifier.focusRequester(categoryFocus).contentEntryFocus(),
						contentFocus = pageContentFocus,
						selectOnFocus = isTvDevice()
					)
					VerticalDivider()
					AppSettingsContent(
						viewModel = viewModel,
						pageId = selectedPage,
						query = query,
						showMessage = ::showMessage,
						modifier = Modifier
							.weight(1f)
							.fillMaxSize()
							.focusRequester(pageContentFocus)
							.focusRestorer()
					)
				}
			} else if (!pageOpen && !searching) {
				AppSettingsPageList(
					entries = entries,
					onOpen = { index ->
						selectedPage = pageIdAt(index)
						pageOpen = true
					},
					modifier = Modifier
						.fillMaxSize()
						.focusRequester(categoryFocus)
						.contentEntryFocus()
				)
			} else {
				// A page, or the search results over the whole of the settings.
				AppSettingsContent(
					viewModel = viewModel,
					pageId = if (searching) null else selectedPage,
					query = query,
					showMessage = ::showMessage,
					modifier = Modifier
						.fillMaxSize()
						.focusRequester(pageContentFocus)
						.focusRestorer()
				)
			}
		}
	}
}

private fun pageIdAt(index: Int): String =
	if (index == PAGES.size) ABOUT_PAGE else PAGES[index].id

/** The phone's list of the pages and About & Licences; tapping (or OK on) a row opens it. */
@Composable
private fun AppSettingsPageList(
	entries: List<SidePaneEntry>,
	onOpen: (Int) -> Unit,
	modifier: Modifier = Modifier
) {
	val firstRowFocus = remember { FocusRequester() }
	Column(
		modifier = modifier
			.verticalScroll(rememberScrollState())
			.focusRestorer(firstRowFocus)
			.padding(horizontal = 16.dp, vertical = 8.dp),
		verticalArrangement = Arrangement.spacedBy(4.dp)
	) {
		entries.forEachIndexed { index, entry ->
			Row(
				verticalAlignment = Alignment.CenterVertically,
				modifier = Modifier
					.fillMaxWidth()
					.then(if (index == 0) Modifier.focusRequester(firstRowFocus) else Modifier)
					.dpadFocusIndicator()
					.clickable(role = Role.Button) { onOpen(index) }
					.padding(vertical = 12.dp)
					.testTag(entry.testTag.orEmpty())
			) {
				Text(entry.label, style = MaterialTheme.typography.titleMedium)
			}
		}
	}
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun AppSettingsSearchRow(
	query: String,
	onQueryChange: (String) -> Unit,
	onClose: () -> Unit
) {
	val typingGate = rememberDpadTypingGate()
	Surface(color = MaterialTheme.colorScheme.surface) {
		Row(
			modifier = Modifier
				.fillMaxWidth()
				.padding(start = 16.dp, end = 8.dp, top = 4.dp, bottom = 4.dp),
			verticalAlignment = Alignment.CenterVertically
		) {
			OutlinedTextField(
				value = query,
				onValueChange = onQueryChange,
				placeholder = { Text(stringResource(R.string.appsettings_search_prompt)) },
				singleLine = true,
				readOnly = typingGate.readOnly,
				modifier = Modifier
					.weight(1f)
					.dpadVerticalMovesFocus()
					.then(typingGate.modifier),
				trailingIcon = {
					if (query.isNotEmpty()) {
						IconButton(
							onClick = { onQueryChange("") },
							modifier = Modifier.dpadFocusIndicator()
						) {
							Icon(Icons.Default.Close, contentDescription = stringResource(R.string.action_clear_search))
						}
					}
				}
			)
			IconButton(onClick = onClose, modifier = Modifier.dpadFocusIndicator()) {
				Icon(Icons.Default.Close, contentDescription = stringResource(R.string.action_cancel))
			}
		}
	}
}

/**
 * One page's settings — every group's rows in the layer's order, the
 * launcher's own settings in their cards — or, with a query, every host
 * setting that matches across the pages, a section per page.
 */
@Composable
private fun AppSettingsContent(
	viewModel: AppSettingsViewModel,
	pageId: String?,
	query: String,
	showMessage: (String) -> Unit,
	modifier: Modifier = Modifier
) {
	val context = LocalContext.current
	val preferences = remember { AppPreferences.getInstance(context) }
	val schema = viewModel.schema
	Column(
		modifier = modifier
			.verticalScroll(rememberScrollState())
			.padding(horizontal = 16.dp, vertical = 16.dp),
		verticalArrangement = Arrangement.spacedBy(16.dp)
	) {
		if (schema == null) {
			Text(stringResource(R.string.appsettings_unavailable), color = MaterialTheme.colorScheme.onSurfaceVariant)
			return@Column
		}
		if (pageId == ABOUT_PAGE) {
			AboutPage(showMessage = showMessage)
			return@Column
		}
		val pages = when (pageId) {
			null -> PAGES.filter { page -> page.groups.any { group -> schema.hostSettings.any { it.group == group && it.matches(query) } } }
			else -> PAGES.filter { it.id == pageId }
		}
		pages.forEach { page ->
			if (pageId == null) {
				Text(stringResource(page.titleRes), style = MaterialTheme.typography.titleMedium)
			}
			page.groups.forEach { group ->
				HostGroupSection(group, viewModel, query)
			}
			LauncherSettings(page = page.id, preferences = preferences, viewModel = viewModel)
		}
		if (pageId == null && pages.isEmpty()) {
			Text(stringResource(R.string.appsettings_search_none), color = MaterialTheme.colorScheme.onSurfaceVariant)
		}
	}
}

/** The settings of one host settings' group, in the layer's order, narrowed by [query]. */
@Composable
private fun HostGroupSection(group: String, viewModel: AppSettingsViewModel, query: String) {
	val options = viewModel.schema?.hostSettings.orEmpty()
		.filter { it.group == group && it.matches(query) }
	if (options.isEmpty()) return
	SettingsSection(title = stringResource(groupTitle(group))) {
		options.forEach { option ->
			HostSettingRow(
				option = option,
				value = viewModel.values[option.id] ?: option.defaultOnAndroid,
				onSet = { value -> viewModel.set(option.id, value) },
				shaderCatalog = viewModel.shaderCatalogEntries,
				bezels = viewModel.bezelEntries
			)
		}
	}
}

/** Whether the setting's label, help or key holds every word of [query]. */
private fun SettingsSchema.Option.matches(query: String): Boolean {
	if (query.isBlank()) return true
	val text = "$label $help $id".lowercase()
	return query.lowercase().split(Regex("\\s+")).filter { it.isNotBlank() }.all { text.contains(it) }
}

/**
 * The launcher's own app-level settings, in the page they belong: the
 * theme and the Library's state under General, the remembered controls
 * under Defaults, the storage location under Folders. Everything a machine
 * of its own sets stays in the machine editor.
 */
@Composable
private fun LauncherSettings(page: String, preferences: AppPreferences, viewModel: AppSettingsViewModel) {
	when (page) {
		"general" -> GeneralLauncherSettings(preferences)
		"defaults" -> RememberedControlsSetting(preferences)
		"folders" -> StorageLocationSetting()
	}
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun GeneralLauncherSettings(preferences: AppPreferences) {
	val context = LocalContext.current

	OutlinedCard(modifier = Modifier.fillMaxWidth()) {
		Column(modifier = Modifier.padding(16.dp)) {
			Text(stringResource(R.string.settings_display_app_theme), style = MaterialTheme.typography.titleMedium)
			Spacer(modifier = Modifier.height(8.dp))

			val themeMode by preferences.themeMode
			val themeModeOptions = listOf(
				"system" to stringResource(R.string.settings_display_theme_system),
				"light" to stringResource(R.string.settings_display_theme_light),
				"dark" to stringResource(R.string.settings_display_theme_dark)
			)
			var themeExpanded by remember { mutableStateOf(false) }
			ExposedDropdownMenuBox(
				expanded = themeExpanded,
				onExpandedChange = { themeExpanded = it }
			) {
				OutlinedTextField(
					value = themeModeOptions.firstOrNull { it.first == themeMode }?.second ?: themeMode,
					onValueChange = {},
					readOnly = true,
					label = { Text(stringResource(R.string.settings_display_theme_label)) },
					trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = themeExpanded) },
					modifier = Modifier
						.dpadFieldFocusIndicator()
						.dpadArrowsMoveFocus()
						.menuAnchor(ExposedDropdownMenuAnchorType.PrimaryNotEditable)
						.fillMaxWidth()
				)
				ExposedDropdownMenu(
					expanded = themeExpanded,
					onDismissRequest = { themeExpanded = false }
				) {
					themeModeOptions.forEach { (value, label) ->
						DropdownMenuItem(
							text = { Text(label) },
							modifier = Modifier.dpadMenuItemInitialFocus(value == themeMode),
							onClick = {
								preferences.setThemeMode(value)
								themeExpanded = false
							}
						)
					}
				}
			}

			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
				Spacer(modifier = Modifier.height(8.dp))
				val useDynamicColor by preferences.useDynamicColor
				SettingSwitchRow(
					label = stringResource(R.string.settings_display_dynamic_color),
					checked = useDynamicColor,
					onCheckedChange = { preferences.setDynamicColor(it) }
				)
			}
		}
	}

	OutlinedCard(modifier = Modifier.fillMaxWidth()) {
		Column(modifier = Modifier.padding(16.dp)) {
			Text(stringResource(R.string.settings_display_in_game_menu), style = MaterialTheme.typography.titleMedium)
			Spacer(modifier = Modifier.height(8.dp))
			val useClassicMenu by preferences.useClassicInGameMenu
			SettingSwitchRow(
				label = stringResource(R.string.settings_display_classic_menu),
				checked = useClassicMenu,
				onCheckedChange = { preferences.setClassicInGameMenu(it) },
				help = stringResource(R.string.settings_display_classic_menu_help)
			)
		}
	}

	OutlinedCard(modifier = Modifier.fillMaxWidth()) {
		Column(modifier = Modifier.padding(16.dp)) {
			Text(stringResource(R.string.appsettings_library_title), style = MaterialTheme.typography.titleMedium)
			Spacer(modifier = Modifier.height(8.dp))
			val romAdviceDismissed by preferences.romAdviceDismissed
			SettingSwitchRow(
				label = stringResource(R.string.appsettings_show_rom_advice),
				checked = !romAdviceDismissed,
				onCheckedChange = { show ->
					if (show) preferences.showRomAdviceAgain() else preferences.dismissRomAdvice()
				},
				help = stringResource(R.string.appsettings_show_rom_advice_help)
			)
			Spacer(modifier = Modifier.height(8.dp))
			TextButton(
				onClick = {
					preferences.clearRecentLaunches()
				},
				modifier = Modifier.fillMaxWidth().dpadFocusIndicator()
			) {
				Text(stringResource(R.string.appsettings_clear_recent))
			}
		}
	}
}

/** The controls machines leave behind, which the next new machine starts from: one reset. */
@Composable
private fun RememberedControlsSetting(preferences: AppPreferences) {
	OutlinedCard(modifier = Modifier.fillMaxWidth()) {
		Column(modifier = Modifier.padding(16.dp)) {
			Text(stringResource(R.string.appsettings_remembered_controls_title), style = MaterialTheme.typography.titleMedium)
			Spacer(modifier = Modifier.height(8.dp))
			Text(
				stringResource(R.string.appsettings_remembered_controls_help),
				style = MaterialTheme.typography.bodyMedium,
				color = MaterialTheme.colorScheme.onSurfaceVariant
			)
			Spacer(modifier = Modifier.height(8.dp))
			TextButton(
				onClick = { preferences.resetRememberedControls() },
				modifier = Modifier.fillMaxWidth().dpadFocusIndicator()
			) {
				Text(stringResource(R.string.appsettings_reset_remembered_controls))
			}
		}
	}
}

/** Where the launcher's content lives and whether it may reach all of it. */
@Composable
private fun StorageLocationSetting() {
	val context = LocalContext.current
	OutlinedCard(modifier = Modifier.fillMaxWidth()) {
		Column(modifier = Modifier.padding(16.dp)) {
			Text(stringResource(R.string.appsettings_storage_title), style = MaterialTheme.typography.titleMedium)
			Spacer(modifier = Modifier.height(8.dp))
			val fullAccess = hasFullStorageAccess(context)
			Text(
				stringResource(
					if (fullAccess) R.string.appsettings_storage_full_access else R.string.appsettings_storage_limited_access
				),
				style = MaterialTheme.typography.bodyMedium
			)
			context.getExternalFilesDir(null)?.let { external ->
				Spacer(modifier = Modifier.height(8.dp))
				Text(
					text = external.absolutePath,
					style = MaterialTheme.typography.bodySmall,
					color = MaterialTheme.colorScheme.onSurfaceVariant
				)
			}
		}
	}
}
