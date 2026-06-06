package com.blitterstudio.amiberry.data

import androidx.annotation.StringRes
import com.blitterstudio.amiberry.R

object AboutActions {

	enum class LinkId {
		SourceCode,
		Wiki,
		Discord,
		Privacy
	}

	data class ExternalLink(
		val id: LinkId,
		@param:StringRes val labelRes: Int,
		val url: String
	)

	fun externalLinks(): List<ExternalLink> = listOf(
		ExternalLink(
			id = LinkId.SourceCode,
			labelRes = R.string.about_link_github,
			url = "https://github.com/BlitterStudio/amiberry"
		),
		ExternalLink(
			id = LinkId.Wiki,
			labelRes = R.string.about_link_wiki,
			url = "https://github.com/BlitterStudio/amiberry/wiki"
		),
		ExternalLink(
			id = LinkId.Discord,
			labelRes = R.string.about_link_discord,
			url = "https://discord.gg/wWndKTGpGV"
		),
		ExternalLink(
			id = LinkId.Privacy,
			labelRes = R.string.about_link_privacy,
			url = "https://amiberry.com/privacy-policy"
		)
	)

	fun openLinkFailedMessage(): ConfigurationActions.Message =
		ConfigurationActions.Message(R.string.msg_link_open_failed)

	fun supportInfoCopiedMessage(): ConfigurationActions.Message =
		ConfigurationActions.Message(R.string.msg_support_info_copied)
}
