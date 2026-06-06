package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.R
import org.junit.Assert.assertEquals
import org.junit.Test

class AboutActionsTest {

	@Test
	fun `external links are stable and user visible`() {
		assertEquals(
			listOf(
				AboutActions.ExternalLink(
					id = AboutActions.LinkId.SourceCode,
					labelRes = R.string.about_link_github,
					url = "https://github.com/BlitterStudio/amiberry"
				),
				AboutActions.ExternalLink(
					id = AboutActions.LinkId.Wiki,
					labelRes = R.string.about_link_wiki,
					url = "https://github.com/BlitterStudio/amiberry/wiki"
				),
				AboutActions.ExternalLink(
					id = AboutActions.LinkId.Discord,
					labelRes = R.string.about_link_discord,
					url = "https://discord.gg/wWndKTGpGV"
				),
				AboutActions.ExternalLink(
					id = AboutActions.LinkId.Privacy,
					labelRes = R.string.about_link_privacy,
					url = "https://amiberry.com/privacy-policy"
				)
			),
			AboutActions.externalLinks()
		)
	}

	@Test
	fun `about action messages are user visible`() {
		assertEquals(
			ConfigurationActions.Message(R.string.msg_link_open_failed),
			AboutActions.openLinkFailedMessage()
		)
		assertEquals(
			ConfigurationActions.Message(R.string.msg_support_info_copied),
			AboutActions.supportInfoCopiedMessage()
		)
	}
}
