package com.blitterstudio.amiberry.data.model

import com.blitterstudio.amiberry.ui.screens.settings.SettingsControlAvailability
import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Test
import java.io.File

/**
 * The launcher's lookups answer what the export wrote: every state of the
 * bundled schema's CPU availability and transition tables, read from the
 * raw JSON and asked through EmulatorSettings.
 */
class SchemaTablesTest {

	private val json = JSONObject(File(requireNotNull(System.getProperty("amiberry.settingsSchema"))).readText())

	/** Every combination of the inputs, the last input changing fastest. */
	private fun states(inputs: JSONArray): List<Map<String, String>> {
		var states = listOf(emptyMap<String, String>())
		for (i in 0 until inputs.length()) {
			val input = inputs.getJSONObject(i)
			val values = input.getJSONArray("values")
			states = states.flatMap { state -> List(values.length()) { state + (input.getString("id") to values.getString(it)) } }
		}
		return states
	}

	private fun JSONArray.strings() = List(length()) { getString(it) }

	/** The launcher settings of a CPU state, with the other transition fields at values the setters may change. */
	private fun settingsIn(state: Map<String, String>) = EmulatorSettings(
		cpuModel = state.getValue("cpu_model").toInt(),
		address24Bit = state.getValue("cpu_24bit_addressing") == "true",
		cycleExact = state.getValue("cycle_exact"),
		jitCacheSize = state.getValue("cachesize").toInt(),
		cpuCompatible = state.getValue("cpu_compatible") == "true",
		fpuModel = state.getValue("fpu_model").toInt(),
		cpuSpeed = "max",
		immediateBlits = true,
		jitFpu = false
	)

	private fun fieldsOf(s: EmulatorSettings) = mapOf(
		"cpu_model" to s.cpuModel.toString(),
		"cpu_24bit_addressing" to s.address24Bit.toString(),
		"cycle_exact" to s.cycleExact,
		"cachesize" to EmulatorSettings.jitState(s.jitCacheSize),
		"cpu_compatible" to s.cpuCompatible.toString(),
		"fpu_model" to s.fpuModel.toString(),
		"cpu_speed" to s.cpuSpeed,
		"immediate_blits" to s.immediateBlits.toString(),
		"compfpu" to s.jitFpu.toString()
	)

	@Test
	fun `every CPU state reads the availability the export wrote for it`() {
		val table = json.getJSONArray("availability").getJSONObject(0)
		val options = table.getJSONArray("options").strings()
		val choices = table.getJSONArray("choices")
		val rows = table.getJSONArray("rows")
		val states = states(table.getJSONArray("inputs"))
		assertEquals(states.size, rows.length())
		states.forEachIndexed { r, state ->
			val settings = settingsIn(state)
			val row = rows.getJSONArray(r)
			options.forEachIndexed { bit, id ->
				val expected = row.getLong(0) and (1L shl bit) != 0L
				assertEquals("$state $id", expected, SettingsControlAvailability.isAvailable(settings, id))
			}
			for (c in 0 until choices.length()) {
				val id = choices.getJSONObject(c).getString("id")
				val values = choices.getJSONObject(c).getJSONArray("values").strings()
				val mask = row.getLong(1 + c)
				val expected = values.filterIndexed { bit, _ -> mask and (1L shl bit) != 0L }
				assertEquals("$state $id", expected, SettingsControlAvailability.choiceValues(settings, id))
			}
		}
	}

	@Test
	fun `every CPU edit makes the changes the export recorded`() {
		val table = json.getJSONObject("transitions")
		val fields = table.getJSONArray("fields").strings()
		val actions = table.getJSONArray("actions")
		val columns = (0 until actions.length()).flatMap { a ->
			val action = actions.getJSONObject(a)
			action.getJSONArray("values").strings().map { action.getString("id") to it }
		}
		val outcomes = table.getJSONArray("outcomes")
		val rows = table.getJSONArray("rows")
		val states = states(table.getJSONArray("inputs"))
		assertEquals(states.size, rows.length())
		states.forEachIndexed { r, state ->
			val before = settingsIn(state)
			columns.forEachIndexed { column, (id, value) ->
				val outcome = rows.getJSONArray(r).getInt(column)
				val after = before.withOption(id, value)
				if (outcome < 0) {
					assertEquals("$state $id=$value is refused", before, after)
					return@forEachIndexed
				}
				val expected = fieldsOf(before).toMutableMap()
				expected[id] = value
				val writes = outcomes.getJSONArray(outcome)
				for (i in 0 until writes.length() step 2) expected[fields[writes.getInt(i)]] = writes.getString(i + 1)
				assertEquals("$state $id=$value", expected, fieldsOf(after))
			}
		}
	}
}
