#include <core.h>
#include <time.h>


Time time_now() {
    struct timespec time_spec_now;
    clock_gettime(CLOCK_REALTIME, &time_spec_now);
    S64 ns = (S64)time_spec_now.tv_sec * 1000000000LL + time_spec_now.tv_nsec;
    return (Time){.nsec = ns};
}
