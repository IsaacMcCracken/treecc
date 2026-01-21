#include <front/front.h>
#include <os/os_inc.h>

typedef struct {
    int _res;
} State;

State s = { 0 };

int frontend_init(void) {
    tokenizer_init();
}

void frontend_deinit(void) {

}
