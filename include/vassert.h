#ifndef __VASSERT_H__
#define __VASSERT_H__

void vassert_impl(const char* exception, const char* filename, int linenum, void* ra, const char* fmt, ...);
// #define vassert(exception, fmt, ...)  ((exception)?((void)0):vassert_impl( # exception , __FILE__, __LINE__, __builtin_return_address(0), fmt, ##__VA_ARGS__))
#define vassert(exception, fmt, ...)

#endif
