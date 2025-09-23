#if defined (_WIN32)
#define PLATFORM "Windows"
#include "windows/os.c"
#else
#define PLATFORM "Posix"
#include "posix/os.c"
#endif
