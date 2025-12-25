#if defined (CORE_PLATFORM_WINDOWS)
#define OS_CSTRING "Windows"
#define OS_STRING (String){"Windows", 7}
#include "windows/os.c"
#else
#define PLATFORM "Posix"
#include "posix/os.c"
#endif
