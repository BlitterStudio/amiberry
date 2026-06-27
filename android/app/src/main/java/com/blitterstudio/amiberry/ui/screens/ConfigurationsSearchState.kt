package com.blitterstudio.amiberry.ui.screens

import com.blitterstudio.amiberry.data.ConfigInfo

enum class ConfigurationListFilter {
	All,
	WithDescription,
	Recent;

	companion object {
		fun fromName(name: String): ConfigurationListFilter =
			entries.firstOrNull { it.name == name } ?: All
	}
}

internal fun shouldShowConfigurationSearch(configCount: Int): Boolean =
	configCount > 5

internal fun filterConfigurations(
	configs: List<ConfigInfo>,
	query: String,
	filter: ConfigurationListFilter
): List<ConfigInfo> {
	val filtered = when (filter) {
		ConfigurationListFilter.All -> configs
		ConfigurationListFilter.WithDescription -> configs.filter { it.description.isNotBlank() }
		ConfigurationListFilter.Recent -> {
			val newest = configs.maxOfOrNull { it.lastModified } ?: return emptyList()
			configs.filter { newest - it.lastModified <= RECENT_WINDOW_MS }
		}
	}
	val normalizedQuery = query.trim().lowercase()
	if (normalizedQuery.isBlank()) {
		return if (filter == ConfigurationListFilter.All) configs else filtered
	}
	return filtered.filter { config ->
		config.name.contains(normalizedQuery, ignoreCase = true) ||
			config.description.contains(normalizedQuery, ignoreCase = true) ||
			config.path.contains(normalizedQuery, ignoreCase = true)
	}
}

private const val RECENT_WINDOW_MS = 7L * 24L * 60L * 60L * 1000L
