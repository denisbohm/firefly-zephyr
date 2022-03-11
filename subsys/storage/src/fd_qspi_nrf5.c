#include <zephyr.h>
#include <device.h>

#include <nrf.h>

static int fd_qspi_nrf5_setup(const struct device *unused) {
    ARG_UNUSED(unused);

    NRF_QSPI_S->ADDRCONF = 0x01000000 | CONFIG_FIREFLY_SUBSYS_STORAGE_QSPI_ENTER_FOUR_BYTE_ADDRESSING_MODE_OPCODE;
    return 0;
}

SYS_INIT(fd_qspi_nrf5_setup, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
