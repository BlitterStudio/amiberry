---
title: "Reliable Recursive Android SAF ROM Folder Imports"
date: 2026-07-30
category: design-patterns
module: "Android file management"
problem_type: design_pattern
component: service_object
severity: medium
applies_when:
  - "Importing a user-selected file collection through an Android DocumentsProvider tree"
  - "Traversing nested SAF directories that may fail after returning partial results"
  - "Using provider metadata to validate and sanitize local filenames"
tags:
  - "android"
  - "storage-access-framework"
  - "documents-provider"
  - "document-tree"
  - "rom-import"
  - "recursive-traversal"
  - "partial-failure"
  - "filename-sanitization"
---

# Reliable Recursive Android SAF ROM Folder Imports

## Context

Android ROM import needs to support selecting either individual Kickstart files or one directory containing a nested ROM collection. The directory flow starts with `OpenDocumentTree` and passes the selected tree URI into the ROM importer (`android/app/src/main/java/com/blitterstudio/amiberry/ui/screens/FileManagerScreen.kt:151` and `android/app/src/main/java/com/blitterstudio/amiberry/ui/viewmodel/FileManagerViewModel.kt:60`).

Storage Access Framework trees are not ordinary filesystem directories. The importer is designed and tested to tolerate null query results, enumeration exceptions, repeated directory IDs, and display names that differ from document IDs. A reliable importer must preserve those distinctions instead of treating every missing query result as an empty directory.

An early implementation correctly discovered nested documents, but it logged and discarded directory-query failures. That made an unreadable root look empty and made an unreadable nested directory look like a completely successful partial scan. It also queried each leaf again for a display name already available in the child-directory cursor.

## Guidance

### Return documents and traversal failures together

Model tree enumeration as a result containing both discovered documents and failed directories. Amiberry uses `DocumentTreeTraversal` for this boundary (`android/app/src/main/java/com/blitterstudio/amiberry/data/FileManager.kt:62`).

The traversal preserves a thrown query exception or null cursor before continuing:

```kotlin
val children = try {
	queryChildren(documentId)
} catch (e: Exception) {
	failures += DocumentTreeFailure(documentId, e)
	continue
}
if (children == null) {
	failures += DocumentTreeFailure(documentId, null)
	continue
}
```

This logic lives in `traverseDocumentTree()` (`android/app/src/main/java/com/blitterstudio/amiberry/data/FileManager.kt:389`). Documents discovered before a nested failure remain importable, while the failure remains available to the feedback layer.

### Project all downstream metadata in the directory query

Request the document ID, MIME type, and `COLUMN_DISPLAY_NAME` together (`android/app/src/main/java/com/blitterstudio/amiberry/data/FileManager.kt:337`). Carry the display name with the document ID instead of resolving it through another provider query for each leaf.

The folder importer passes the projected name directly into the import path (`android/app/src/main/java/com/blitterstudio/amiberry/data/FileManager.kt:108`). It falls back to the final part of the document ID only when the provider returns a blank name. Individual-file imports can still query `OpenableColumns.DISPLAY_NAME`; the folder path avoids that repeated work.

### Deduplicate directory IDs

Use a queue for pending directories and a visited-ID set for cycle protection. Amiberry skips an ID after its first query (`android/app/src/main/java/com/blitterstudio/amiberry/data/FileManager.kt:394` and `android/app/src/main/java/com/blitterstudio/amiberry/data/FileManager.kt:401`).

Do not assume a provider tree has strict filesystem semantics. Deduplicating IDs prevents repeated queries and guarantees termination if a provider exposes cycles.

### Convert enumeration failures into typed import outcomes

Do not stop at logging. Convert every traversal failure into `ImportResult.Failed` before returning the batch (`android/app/src/main/java/com/blitterstudio/amiberry/data/FileManager.kt:128`).

The feedback layer can then distinguish:

- A genuinely empty tree: no results, reported as no supported files.
- An unreadable tree: one or more failed results.
- A partially readable tree: imported or unsupported files plus failed results.

These branches are derived from typed result counts in `ImportBatchFeedback` (`android/app/src/main/java/com/blitterstudio/amiberry/data/ImportBatchFeedback.kt:20`).

### Sanitize provider-controlled names before copying

Treat a provider display name as untrusted input. Normalize it to a leaf name, replace unsafe characters, strip unsafe edge characters, and validate the sanitized extension against the selected file category (`android/app/src/main/java/com/blitterstudio/amiberry/data/FileManager.kt:235` and `android/app/src/main/java/com/blitterstudio/amiberry/data/FileManager.kt:260`).

Keep name handling at the import boundary. Tree enumeration should preserve provider metadata without deciding whether a file is valid for the destination category.

## Why This Matters

A recursive import is correct only when it preserves the difference between "the directory was empty" and "the directory could not be enumerated." Logging a provider failure without returning it changes user-visible semantics: an incomplete scan can appear successful.

Carrying the projected display name also removes an N+1-style metadata lookup without coupling cursor traversal to file copying. Enumeration owns provider metadata, copying owns streams and destination files, and the traversal result is the explicit boundary between them.

Typed outcomes let the existing UI report empty, failed, partial, and successful imports without inferring state from exceptions or log messages.

## When to Apply

- A user selects an Android directory through `OpenDocumentTree`.
- Files may exist below multiple levels of provider-backed directories.
- Partial results are useful even if one nested query fails.
- Provider metadata is needed for filtering, display, or destination naming.
- The provider may expose duplicate IDs or non-filesystem tree behavior.

## Examples

The traversal regression suite keeps four cases together in `android/app/src/test/java/com/blitterstudio/amiberry/data/DocumentTreeTraversalTest.kt`:

- Nested traversal preserves projected names (`:12`).
- A null root cursor becomes a failure rather than an empty tree (`:43`).
- A nested exception keeps previously discovered files and reports partial traversal (`:53`).
- Duplicate directory IDs are queried once (`:76`).

This session recorded successful runs of the full `:app:testDebugUnitTest` suite, `:app:lintDebug`, and `:app:assembleDebug` for `arm64-v8a` and `x86_64`, plus `git diff --check`. The session did not include a physical-device folder-picker test; device verification should cover an empty folder, nested ROMs, unsupported files, duplicate filenames, and access loss during traversal.

## Related

No existing solution document covered Android SAF tree import when this learning was captured.
