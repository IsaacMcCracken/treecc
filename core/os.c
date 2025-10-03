#if defined (CORE_PLATFORM_WINDOWS)
#define PLATFORM_CSTRING "Windows"
#define PLATFORM_STRING (String){"Windows", 7}
#include "windows/os.c"
#else
#define PLATFORM "Posix"
#include "posix/os.c"
#endif
