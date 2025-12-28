#ifndef CORE_INTERNAL_H
#define CORE_INTERNAL_H

Thread os_thread_launch(FnThreadEntry fn, void *ptr);
B32 os_thread_join(Thread thread);

Mutex os_mutex_alloc(void);
void os_mutex_free(Mutex m);

#endif
