#include "lwip/opt.h"
#include "lwip/arch/cc.h"
#include "../timer.h"

void sys_init(void) {
}

void tcpip_try_callback(void (*f)(void *ctx), void *ctx) {
    if (f)
        f(ctx);
}

