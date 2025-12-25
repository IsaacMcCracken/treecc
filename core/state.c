/*
 * This file contains global state that is libc like and should not be
 * given to the user.
 */

 #include <core.h>

S32 os_posix_core_lib_init(void);


 S32 core_init(void) {
     os_posix_core_lib_init();
     return 1;
 }
