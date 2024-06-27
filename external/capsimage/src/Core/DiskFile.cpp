#include "config.h"
#include "stdafx.h"

CDiskFile::CDiskFile()
{
	dfile=NULL;
	lastop=-1;
}

CDiskFile::~CDiskFile()
{
	Close();
}

// open file, return true on error
int CDiskFile::Open(const char *name, unsigned int mode)
{
	// close file if it is open
	Close();

	// check name
	if (!name || !strlen(name))
		return 1;

	// select open mode
	const char *om;

	if (mode & BFFLAG_WRITE) {
		if (mode & BFFLAG_CREATE)
			om="w+b";
		else
			om="r+b";
	} else
		om="rb";

	// open file, stop on error
	if (!(dfile=fopen(name, om)))
		return 1;

	// success
	fileopen=1;
	filemode=mode;

	return 0;
}

// open the first accessible file from the name list
// return the name index 0...n on the first successful attempt, -1 if all failed
int CDiskFile::OpenAny(char **name, unsigned int mode)
{
	int pos;

	// if name list is valid
	if (name) {
		// try each name in order
		for (pos=0; name[pos]; pos++) {
			// open the file, return the name index position on success
			if (!Open(name[pos], mode))
				return pos;
		}
	}

	// all attempts failed
	return -1;
}

// open file from any of the path entries listed
// return the path index 0...n on the first successful attempt, -1 if all failed
int CDiskFile::OpenAnyPath(char **path, const char *name, unsigned int mode)
{
	int pos;

	// if name and path list are valid
	if (name && path) {
		// try each path entry in order
		for (pos=0; path[pos]; pos++) {
			// append name to current path entry
			int len=sprintf(tempname, "%s", path[pos]);
			sprintf(tempname+len, "%s", name);

			// open the file, return the name index position on success
			if (!Open(tempname, mode))
				return pos;
		}
	}

	// all attempts failed
	return -1;
}

// close file, return true on error
int CDiskFile::Close()
{
	// finished if file is not open
	if (!dfile)
		return 0;

	// close file, set error
	int res=fclose(dfile) ? 1 : 0;

	// clear settings
	dfile=NULL;
	lastop=-1;
	Clear();

	return res;
}

// read file, return number of bytes read
size_t CDiskFile::Read(void *buf, size_t size)
{
	// 0 bytes read if file is not open
	if (!dfile)
		return 0;

	// read/write change
	if (lastop != 0) {
		fseek(dfile, 0, SEEK_CUR);
		lastop=0;
	}

	// read the file
	return fread(buf, 1, size, dfile);
}

// write file, return number of bytes written
size_t CDiskFile::Write(void *buf, size_t size)
{
	// 0 bytes written if file is not open or read-only
	if (!dfile || !(filemode & BFFLAG_WRITE))
		return 0;

	// read/write change
	if (lastop != 1) {
		fseek(dfile, 0, SEEK_CUR);
		lastop=1;
	}

	// read the file
	return fwrite(buf, 1, size, dfile);
}

// seek in file, return the current position
long CDiskFile::Seek(long pos, int mode)
{
	// default result 
	long res=0;

	// return default result if file is not open
	if (!dfile)
		return res;

	// select seek mode
	int sm;

	switch (mode) {
		case Start:
			pos=0;
			sm=SEEK_SET;
			break;

		case Position:
			sm=SEEK_SET;
			break;

		case Current:
			sm = SEEK_CUR;
			break;

		case End:
			pos=0;
			sm=SEEK_END;
			break;

		default:
			return res;
	}

	// seek, stop on error
	if (fseek(dfile, pos, sm))
		return res;

	// get the current file position, stop on error
	pos=ftell(dfile);
	if (pos < 0)
		return res;

	return pos;
}

// get the file size
long CDiskFile::GetSize()
{
	// default result 
	long res=0;

	// return default result if file is not open
	if (!dfile)
		return res;

	// get the current file position, stop on error
	long pos=ftell(dfile);
	if (pos < 0)
		return res;

	// get the file size
	long size=Seek(0, End);

	// restore file position, stop on error
	if (Seek(pos, Position) != pos)
		return res;

	return size;
}

// get file position
long CDiskFile::GetPosition()
{
	// default result 
	long res=0;

	// return default result if file is not open
	if (!dfile)
		return res;

	long pos=ftell(dfile);
	if (pos < 0)
		return res;

	return pos;
}

// get the file buffer
uint8_t *CDiskFile::GetBuffer()
{
	// not supported
	return NULL;
}

// create the full path required for writing the file specified
void CDiskFile::MakePath(const char *filename)
{
	if (!filename)
		return;

	char path[MAX_FILENAMELEN];

	// create each directory level delimited by / or \ if does not exist
	for (int rpos = 0, wpos = 0; filename[rpos]; rpos++) {
		if (filename[rpos] == '/' || filename[rpos] == '\\') {
			path[wpos] = 0;

			if (_access(path, 0) == -1)
				_mkdir(path);
		}

		path[wpos++] = filename[rpos];
	}
}

// use filename as is, or resolve to an existing file if it contains wildcards with optional name filter
// return 1 if result is valid
int CDiskFile::FindFile(char *result, const char *filename, const char *filter)
{
	// result is not valid by default
	int resvalid = 0;

	// copy source as result by default
	int rescopy = 1;

	// stop if no buffer specified for the result
	if (!result)
		return resvalid;

	// the default result is an empty string
	result[0] = 0;

	// stop if the filename is not specified
	if (!filename)
		return resvalid;

	// length of path and file name components separated
	int pathlen = 0, namelen = 0;

	// wildcard usage counter, wildcards enabled
	int wcused = 0;
	int wcenabled = 1;

	// process the entire source filename or pattern
	for (int rpos = 0; wcenabled && filename[rpos]; rpos++) {
		namelen++;

		switch (filename[rpos]) {
			// if path delimiter found
		case '/':
		case '\\':
			// stop if a wilcard has already been used; only allow wildcards in filename part, not in the path
			if (wcused) {
				// disable write card processing; the result will be the path upto the illegal wildcard segment
				wcenabled = 0;
				break;
			}

			// set new path length as the entire name until now including the delimiter
			pathlen = rpos + 1;

			// clear the filename part
			namelen = 0;
			break;

			// count the wildcards
		case '?':
		case '*':
			wcused++;
			break;
		}
	}

	// if wildcards used
	if (wcused) {
		// and wilcardcards are enabled, ie no error in the path
		if (wcenabled) {
			char *pathbuf = NULL;
			const char *dirpath;

			// make a copy of the path if it's defined or use the current directory as the path
			if (pathlen) {
				pathbuf = new char[pathlen + 1];
				memcpy(pathbuf, filename, pathlen);
				pathbuf[pathlen] = 0;
				dirpath = pathbuf;
			} else
				dirpath = ".";

			// open the selected path
			DIR *pdir = opendir(dirpath);
			if (pdir) {
				dirent *pent;
				const char *pattern = filename + pathlen;

				// iterate all directory entries
				while ((pent = readdir(pdir)) != NULL) {
					// skip any entry that is not a regular file
#if defined _DIRENT_HAVE_D_TYPE || defined HAVE_STRUCT_DIRENT_D_TYPE
					if (pent->d_type != DT_REG)
						continue;
#endif
					// check entry to match the filename pattern
					char *fn = pent->d_name;
					if (FileNameMatch(pattern, fn)) {
						// if matches, check entry to match the filter pattern if filter is specified
						if (!filter || FileNameMatch(filter, fn)) {
							// success; copy the original path and add the entry found
							memcpy(result, filename, pathlen);
							strcpy(result + pathlen, fn);

							// keep this result, don't copy the source
							rescopy = 0;

							// result is valid
							resvalid = 1;
							break;
						}
					}
				}

				closedir(pdir);
			}

			// free path buffer
			delete[] pathbuf;
		}
	} else {
		// wilcards unused, result is valid
		resvalid = 1;
	}

	// copy the source if that's the result
	if (rescopy) {
		int reslen = pathlen + namelen;
		memcpy(result, filename, reslen);
		result[reslen] = 0;
	}

	return resvalid;
}

// return true if filename matches the wildcard pattern
int CDiskFile::FileNameMatch(const char *pattern, const char *filename)
{
	// an undefined pattern or filename never matches
	if (!pattern || !filename)
		return 0;

	// starmode is true if the pattern starts with a *
	int starmode = 0;

	// fold multiple consecutive stars into a single one - the meaning is identical
	while (pattern[0] == '*') {
		// pattern has at least a *
		starmode = 1;

		// the leading * gets removed, ie the pattern never includes any *
		pattern++;
	}

	// length of the current pattern
	int patternlength = 0;

	// find the length of the current pattern; either the remaining string or the string until the next * 
	while (pattern[patternlength]) {
		if (pattern[patternlength] == '*')
			break;

		patternlength++;
	}

	// always match if the pattern segment is empty and starts with *
	if (!patternlength && starmode)
		return 1;

	// length of the filename
	int namelength = (int)strlen(filename);

	// no match if pattern segment is empty, but the remaining filename is not
	if (!patternlength && namelength)
		return 0;

	// Now the actual pattern segment only contains valid characters or ?
	// Try to match the pattern segment with the actual remaining filename from the beginning.
	// If this fails, try the next position until the filename ends.
	for (int namepos = 0; namepos < namelength; namepos++) {
		// no match if the pattern segment is longer than the remaining filename
		if (patternlength > namelength - namepos)
			return 0;

		int patpos;

		// compare each character of the pattern segment with the remaining filename
		for (patpos = 0; patpos < patternlength; patpos++) {
			// ? matches any character
			if (pattern[patpos] == '?')
				continue;

			// case insensitive comparison; stop if the characters are different
			if (tolower((unsigned char)pattern[patpos]) != tolower((unsigned char)filename[patpos+namepos]))
				break;
		}

		if (patpos >= patternlength) {
			// If the pattern segment fully matches:
			// remove the pattern segment from the pattern
			pattern += patternlength;

			// remove the matched segment from the filename
			filename += patternlength + namepos;

			// calculate the remaining filename length 
			namelength -= patternlength + namepos;

			// empty pattern segment
			patternlength = 0;

			// If the pattern has more characters, recursively match the remaining pattern with the remaining filename
			if (pattern[0])
				return FileNameMatch(pattern, filename);
			else
				break;
		} else {
			// The pattern does not match the filename at the current position.
			// Fail, if the pattern segment did not start with a * allowing the segment to match from any position
			// Otherwise, try the comparison from the next character in the filename
			if (!starmode)
				return 0;
		}
	}

	// All processing done; match if the pattern and the name have no characters left, fail otherwise.
	return !patternlength && !namelength;
}
