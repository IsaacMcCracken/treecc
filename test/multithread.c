#include <core.h>
#include <stdio.h>


int main() {
    if (!core_init()) {
        fprintf(stderr, "What the hell happened");
        return -1;
    }
    printf("Hello Mother.");

    return 0;
}
