#include <core.h>
#include <stdio.h>

void *task(void *ptr) {
    S64 *numptr = (S64 *)ptr;
    S64 num = *numptr;
    *numptr = num * num;
    return 0;
}



S32 entry_point(String *args, U64 arg_count) {
    S64 nums[4] = {1, 69, 2, 420};
    Thread threads[4];
    for (U32 i = 0; i < 4; i++) {
        printf("Thread %u (%ld @%p)\n", i, nums[i], &nums[i]);
        threads[i] = os_thread_launch(task, &nums[i]);
    }


    for (U32 i = 0; i < 4; i++) {
        os_thread_join(threads[i], 0);
    }

    S64 sum = 0;
    for (U32 i = 0; i < 4; i++) {
        printf("%ld +", nums[i]);
        sum += nums[i];
    }

    printf("0 = %ld\n", sum);

    OSSystemInfo sys_info = os_get_system_info();
    printf("CPU COUNT: %u", sys_info.cpu_count);
    return 0;
}
