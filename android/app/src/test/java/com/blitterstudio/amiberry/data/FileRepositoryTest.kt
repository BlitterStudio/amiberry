package com.blitterstudio.amiberry.data

import com.blitterstudio.amiberry.data.model.AmigaFile
import com.blitterstudio.amiberry.data.model.FileCategory
import java.util.concurrent.atomic.AtomicInteger
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.delay
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Test

class FileRepositoryTest {

	@Test
	fun `overlapping full rescans are serialized instead of dropped`() = runBlocking {
		val calls = AtomicInteger(0)
		val repository = FileRepository { category ->
			if (calls.incrementAndGet() == 1) {
				Thread.sleep(100)
			}
			listOf(fileFor(category, calls.get()))
		}

		val first = async(Dispatchers.Default) { repository.rescan() }
		delay(10)
		val second = async(Dispatchers.Default) { repository.rescan() }

		first.await()
		second.await()

		assertEquals(FileCategory.entries.size * 2, calls.get())
	}

	private fun fileFor(category: FileCategory, sequence: Int): AmigaFile =
		AmigaFile(
			path = "/tmp/${category.dirName}/$sequence.${category.extensions.first()}",
			name = "$sequence.${category.extensions.first()}",
			extension = category.extensions.first(),
			size = 1L,
			lastModified = sequence.toLong(),
			category = category
		)
}
