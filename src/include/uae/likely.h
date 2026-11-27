#ifndef UAE_LIKELY_H
#define UAE_LIKELY_H

#if defined(HAVE___BUILTIN_EXPECT) || defined(__GNUC__)

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#else

#define likely(x)   x
#define unlikely(x) x

#endif

#endif /* UAE_LIKELY_H */
