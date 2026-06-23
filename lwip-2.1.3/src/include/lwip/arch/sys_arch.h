#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#include "lwip/opt.h"
#include <stdint.h>

#define SYS_ARCH_NULL 0

#if !NO_SYS
typedef int sys_sem_t;
typedef int sys_mutex_t;
typedef int sys_mbox_t;
typedef int sys_thread_t;
#endif

typedef int sys_prot_t;

#undef SYS_ARCH_TIMEOUT
#define SYS_ARCH_TIMEOUT 0xFFFFFFFF

#endif /* LWIP_ARCH_SYS_ARCH_H */
