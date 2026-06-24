#ifndef STREAM64_H
#define STREAM64_H

typedef int64_t off64_t;

// fopen for large files, >= 2GB
inline FILE * fopen64(const char *filename, const char *opentype)
{
	return fopen(filename, opentype);
}

// ftell for large files, >= 2GB
inline off64_t ftello64(FILE *stream)
{
	return _ftelli64(stream);
}

// fseek for large files, >= 2GB
inline int fseeko64(FILE *stream, off64_t offset, int whence)
{
	return _fseeki64(stream, offset, whence);
}

#endif
