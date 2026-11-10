package com.blitterstudio.amiberry.ui.screens.settings

import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.res.stringResource
import com.blitterstudio.amiberry.R
import com.blitterstudio.amiberry.data.model.SettingsSchema
import com.blitterstudio.amiberry.ui.components.settings.ChoiceItem
import com.blitterstudio.amiberry.ui.components.settings.CollapsibleSettingsSection
import com.blitterstudio.amiberry.ui.components.settings.SettingChoiceRow
import com.blitterstudio.amiberry.ui.components.settings.SettingSwitchRow
import com.blitterstudio.amiberry.ui.components.settings.schemaOption
import com.blitterstudio.amiberry.ui.components.settings.settingsSchema
import com.blitterstudio.amiberry.ui.viewmodel.SettingsViewModel

/**
 * The Hardware page's Accelerator board section, collapsed like the expert
 * rows: the board (the schema's accelerator_boards, which also carries the
 * jumpers), its memory (what the board takes) and its boot ROM. Choosing a
 * board sets the CPU it needs (the schema's accelerator transitions) and
 * starts the board's jumpers over.
 */
@Composable
fun AcceleratorBoardSection(viewModel: SettingsViewModel) {
	val sectionTitle = stringResource(R.string.settings_group_accelerator)
	CollapsibleSettingsSection(
		title = sectionTitle,
		expanded = ACCELERATOR_KEY in viewModel.expandedSections,
		onExpandedChange = { expanded -> viewModel.setSectionExpanded(ACCELERATOR_KEY, expanded) },
		content = { AcceleratorSectionContent(viewModel) }
	)
}

/** The section's remembered open state (SettingsGroups.Placement.key). */
internal const val ACCELERATOR_KEY = "accelerator-board"

/** The section's rows: the board, then what the chosen board has. */
@Composable
internal fun AcceleratorSectionContent(viewModel: SettingsViewModel) {
	val schema = settingsSchema() ?: return
	AcceleratorTypeRow(viewModel)
	val board = schema.acceleratorBoard(viewModel.settings.optionValue("cpuboard_type") ?: "none")
	if (board == null) return
	if (board.memoryMax > 0) AcceleratorMemoryRow(viewModel, board)
	AcceleratorRomRow(viewModel)
	for (jumper in board.settings) {
		AcceleratorJumperRow(viewModel, jumper)
	}
}

/** The board (cpuboard_type): the schema's board list, as its own row where All Settings shows it too. */
@Composable
internal fun AcceleratorTypeRow(viewModel: SettingsViewModel) {
	val schema = settingsSchema() ?: return
	val settings = viewModel.settings
	val none = stringResource(R.string.settings_option_none)
	// A build without a feature a board needs (PPC) does not offer it.
	val boards = schema.acceleratorBoards.filter { it.requires.isEmpty() }
	val boardId = settings.optionValue("cpuboard_type") ?: "none"
	val typeOption = schemaOption("cpuboard_type")

	SettingChoiceRow(
		label = typeOption?.label ?: stringResource(R.string.settings_group_accelerator),
		valueText = if (boardId == "none") none else schema.acceleratorBoard(boardId)?.label ?: boardId,
		choices = listOf(ChoiceItem("none", none)) + boards.map { ChoiceItem(it.id, it.label) },
		help = typeOption?.help?.takeIf { it.isNotBlank() },
		onSelect = { value ->
			viewModel.updateSettings { s ->
				// Another board: its jumpers start over, as the layer's setter does.
				s.copy(acceleratorJumpers = emptyMap()).withOption("cpuboard_type", value)
			}
		}
	)
}

/** The board memory (cpuboardmem1_size): what the board takes. */
@Composable
internal fun AcceleratorMemoryRow(viewModel: SettingsViewModel, board: SettingsSchema.AcceleratorBoard) {
	val settings = viewModel.settings
	val memory = schemaOption("cpuboardmem1_size") ?: return
	val stored = settings.optionValue("cpuboardmem1_size") ?: "0"
	val offered = memory.choices.filter { (it.value.toIntOrNull() ?: 0) <= board.memoryMax }
	SettingChoiceRow(
		label = memory.label,
		valueText = memory.choices.firstOrNull { it.value == stored }?.label ?: stored,
		choices = offered.map { ChoiceItem(it.value, it.label) },
		help = memory.help?.takeIf { it.isNotBlank() },
		onSelect = { value -> viewModel.updateSettings { it.withOption("cpuboardmem1_size", value) } }
	)
}

/** The board's boot ROM (cpuboard_rom_file): the scanned ROMs, or none. */
@Composable
internal fun AcceleratorRomRow(viewModel: SettingsViewModel) {
	val settings = viewModel.settings
	val roms by viewModel.availableRoms.collectAsState()
	FileDropdown(
		label = schemaOption("cpuboard_rom_file")?.label ?: stringResource(R.string.settings_group_accelerator),
		files = roms,
		selectedPath = settings.optionValue("cpuboard_rom_file") ?: "",
		onSelect = { path -> viewModel.updateSettings { it.withOption("cpuboard_rom_file", path) } },
		onClear = { viewModel.updateSettings { it.withOption("cpuboard_rom_file", "") } }
	)
}

/** One jumper of the board: a switch, a selector of its positions, or a text field. */
@Composable
private fun AcceleratorJumperRow(viewModel: SettingsViewModel, jumper: SettingsSchema.AcceleratorJumper) {
	val settings = viewModel.settings
	val value = settings.acceleratorJumpers[jumper.name]
	if (jumper.selector) {
		val positions = jumper.positions
		val labels = positions.indices.map { index ->
			ChoiceItem(positions[index], jumper.positionLabels.getOrElse(index) { positions[index] })
		}
		SettingChoiceRow(
			label = jumper.label,
			valueText = jumperLabel(jumper, value),
			choices = labels,
			onSelect = { selected ->
				viewModel.updateSettings {
					it.copy(acceleratorJumpers = it.acceleratorJumpers + (jumper.name to selected))
				}
			}
		)
	} else if (jumper.type == "text") {
		// The boards carry no text jumpers today; a future one shows a field.
		BoardTextFieldRow(jumper.label, value ?: "") { text ->
			viewModel.updateSettings {
				it.copy(acceleratorJumpers = it.acceleratorJumpers + (jumper.name to text))
			}
		}
	} else {
		SettingSwitchRow(
			label = jumper.label,
			checked = value == "true",
			onCheckedChange = { on ->
				viewModel.updateSettings {
					it.copy(acceleratorJumpers = it.acceleratorJumpers + (jumper.name to if (on) "true" else "false"))
				}
			}
		)
	}
}

/** A selector jumper's shown label: its position's name, the first while none is set. */
private fun jumperLabel(jumper: SettingsSchema.AcceleratorJumper, value: String?): String {
	val index = jumper.positions.indexOf(value).takeIf { it >= 0 } ?: 0
	return jumper.positionLabels.getOrNull(index) ?: jumper.positions.getOrNull(index) ?: ""
}
