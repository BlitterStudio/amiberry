package com.blitterstudio.amiberry.data

import android.provider.DocumentsContract
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class DocumentTreeTraversalTest {

	@Test
	fun `traversal preserves projected names across nested directories`() {
		val queries = mutableListOf<String>()
		val children = mapOf(
			"root" to listOf(
				directory("nested", "Kickstarts"),
				file("kick13", "Kickstart 1.3.rom"),
				file("notes", "notes.txt")
			),
			"nested" to listOf(
				file("kick31", "Kickstart 3.1.rom")
			)
		)

		val result = FileManager.traverseDocumentTree("root") { documentId ->
			queries += documentId
			children[documentId]
		}

		assertEquals(listOf("root", "nested"), queries)
		assertEquals(
			listOf(
				FileManager.DocumentTreeDocument("kick13", "Kickstart 1.3.rom"),
				FileManager.DocumentTreeDocument("notes", "notes.txt"),
				FileManager.DocumentTreeDocument("kick31", "Kickstart 3.1.rom")
			),
			result.documents
		)
		assertTrue(result.failures.isEmpty())
	}

	@Test
	fun `null root cursor is preserved as a traversal failure`() {
		val result = FileManager.traverseDocumentTree("root") { null }

		assertTrue(result.documents.isEmpty())
		assertEquals(1, result.failures.size)
		assertEquals("root", result.failures.single().documentId)
		assertNull(result.failures.single().cause)
	}

	@Test
	fun `nested query failure keeps discovered files and reports partial traversal`() {
		val failure = IllegalStateException("provider unavailable")
		val result = FileManager.traverseDocumentTree("root") { documentId ->
			when (documentId) {
				"root" -> listOf(
					directory("nested", "Nested"),
					file("kick13", "Kickstart 1.3.rom")
				)
				"nested" -> throw failure
				else -> emptyList()
			}
		}

		assertEquals(
			listOf(FileManager.DocumentTreeDocument("kick13", "Kickstart 1.3.rom")),
			result.documents
		)
		assertEquals(1, result.failures.size)
		assertEquals("nested", result.failures.single().documentId)
		assertEquals(failure, result.failures.single().cause)
	}

	@Test
	fun `duplicate directory IDs are queried once`() {
		var nestedQueries = 0
		val result = FileManager.traverseDocumentTree("root") { documentId ->
			when (documentId) {
				"root" -> listOf(
					directory("nested", "Nested"),
					directory("nested", "Nested")
				)
				"nested" -> {
					nestedQueries++
					listOf(file("kick31", "Kickstart 3.1.rom"))
				}
				else -> emptyList()
			}
		}

		assertEquals(1, nestedQueries)
		assertEquals(1, result.documents.size)
		assertTrue(result.failures.isEmpty())
	}

	private fun directory(documentId: String, displayName: String) =
		FileManager.DocumentTreeChild(
			documentId = documentId,
			mimeType = DocumentsContract.Document.MIME_TYPE_DIR,
			displayName = displayName
		)

	private fun file(documentId: String, displayName: String) =
		FileManager.DocumentTreeChild(
			documentId = documentId,
			mimeType = "application/octet-stream",
			displayName = displayName
		)
}
