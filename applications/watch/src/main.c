#include "fd_watch.h"

#include <zephyr/arch/cpu.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/reboot.h>

static const char *get_context_name(const z_arch_esf_t *esf) {
    if ((esf->basic.xpsr & IPSR_ISR_Msk) != 0) {
        return "MAIN/ISR";
    } else {
        struct k_thread *thread = k_current_get();
        if (thread != 0) {
            const char *thread_name = k_thread_name_get(thread);
            if (thread_name != 0) {
                return thread_name;
            }
            return "Anonymous Thread";
        }
    }
    return "Unknown Context";
}

static const char *get_reason_name(unsigned int reason) {
	switch (reason) {
        case K_ERR_CPU_EXCEPTION:
            return "CPU Exception";
        case K_ERR_SPURIOUS_IRQ:
            return "Unhandled Interrupt";
        case K_ERR_STACK_CHK_FAIL:
            return "Stack Overflow";
        case K_ERR_KERNEL_OOPS:
            return "Kernel Oops";
        case K_ERR_KERNEL_PANIC:
            return "Kernel Panic";
        default:
            return "Unknown Error";
	}
}

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *esf) {
    const char *context_name __attribute__((unused)) = get_context_name(esf);
    const char *reason_name __attribute__((unused)) = get_reason_name(reason);

#ifdef CONFIG_REBOOT
    sys_reboot(SYS_REBOOT_COLD);
#else
    // loop here so debugger can attach and break -denis
    while (true) {
    }
#endif
}

int main(void) {
    fd_watch_initialize();
    return 0;
}
