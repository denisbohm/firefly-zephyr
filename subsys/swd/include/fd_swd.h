#ifndef fd_swd_h
#define fd_swd_h

#include "fd_swd_channel.h"

extern const uint64_t fd_swd_error_unexpected_ack;
extern const uint64_t fd_swd_error_too_many_wait_retries;
extern const uint64_t fd_swd_error_sticky;
extern const uint64_t fd_swd_error_parity;
extern const uint64_t fd_swd_error_mismatch;
extern const uint64_t fd_swd_error_invalid;

typedef struct {
    uint64_t code;
    uint64_t detail;
} fd_swd_error_t;

bool fd_swd_error_return(fd_swd_error_t *error, uint64_t code, uint64_t detail);

#define FD_SWD_DP_DPIDR     0x00
#define FD_SWD_DP_ABORT     0x00
#define FD_SWD_DP_CTRL      0x04
#define FD_SWD_DP_STAT      0x04
#define FD_SWD_DP_SELECT    0x08
#define FD_SWD_DP_RDBUFF    0x0c
#define FD_SWD_DP_TARGETSEL 0x0c
#define FD_SWD_DP_DLCR      0x14
#define FD_SWD_DP_TARGETID  0x24
#define FD_SWD_DP_DLPIDR    0x34
#define FD_SWD_DP_EVENTSTAT 0x44

#define FD_SWD_BIT(n) (1 << (n))

#define FD_SWD_DP_ABORT_ORUNERRCLR FD_SWD_BIT(4)
#define FD_SWD_DP_ABORT_WDERRCLR FD_SWD_BIT(3)
#define FD_SWD_DP_ABORT_STKERRCLR FD_SWD_BIT(2)
#define FD_SWD_DP_ABORT_STKCMPCLR FD_SWD_BIT(1)
#define FD_SWD_DP_ABORT_DAPABORT FD_SWD_BIT(0)

#define FD_SWD_DP_CTRL_CSYSPWRUPACK FD_SWD_BIT(31)
#define FD_SWD_DP_CTRL_CSYSPWRUPREQ FD_SWD_BIT(30)
#define FD_SWD_DP_CTRL_CDBGPWRUPACK FD_SWD_BIT(29)
#define FD_SWD_DP_CTRL_CDBGPWRUPREQ FD_SWD_BIT(28)
#define FD_SWD_DP_CTRL_CDBGRSTACK FD_SWD_BIT(27)
#define FD_SWD_DP_CTRL_CDBGRSTREQ FD_SWD_BIT(26)
#define FD_SWD_DP_STAT_WDATAERR FD_SWD_BIT(7)
#define FD_SWD_DP_STAT_READOK FD_SWD_BIT(6)
#define FD_SWD_DP_STAT_STICKYERR FD_SWD_BIT(5)
#define FD_SWD_DP_STAT_STICKYCMP FD_SWD_BIT(4)
#define FD_SWD_DP_STAT_TRNMODE (FD_SWD_BIT(3) | FD_SWD_BIT(2))
#define FD_SWD_DP_STAT_STICKYORUN FD_SWD_BIT(1)
#define FD_SWD_DP_STAT_ORUNDETECT FD_SWD_BIT(0)

#define FD_SWD_AP_CSW 0x00
#define FD_SWD_AP_TAR 0x04
#define FD_SWD_AP_SBZ 0x08
#define FD_SWD_AP_DRW 0x0c
#define FD_SWD_AP_BD0 0x10
#define FD_SWD_AP_BD1 0x14
#define FD_SWD_AP_BD2 0x18
#define FD_SWD_AP_BD3 0x1c
#define FD_SWD_AP_DBGDRAR 0xf8
#define FD_SWD_AP_IDR 0xfc

#define FD_SWD_AP_CSW_DBGSWENABLE    0x80000000
#define FD_SWD_AP_CSW_PROT           0x23000000
#define FD_SWD_AP_CSW_SPIDEN         0x00800000
#define FD_SWD_AP_CSW_TR_IN_PROG     0x00000080
#define FD_SWD_AP_CSW_DBG_STATUS     0x00000040
#define FD_SWD_AP_CSW_ADDRINC_SINGLE 0x00000010
#define FD_SWD_AP_CSW_ADDRINC_OFF    0x00000000
#define FD_SWD_AP_CSW_SIZE_32BIT     0x00000002
#define FD_SWD_AP_CSW_SIZE_16BIT     0x00000001
#define FD_SWD_AP_CSW_SIZE_8BIT      0x00000000

#define FD_SWD_MEMORY_CPUID 0xE000ED00
#define FD_SWD_MEMORY_DFSR  0xE000ED30
#define FD_SWD_MEMORY_DHCSR 0xE000EDF0
#define FD_SWD_MEMORY_DCRSR 0xE000EDF4
#define FD_SWD_MEMORY_DCRDR 0xE000EDF8
#define FD_SWD_MEMORY_DEMCR 0xE000EDFC

#define FD_SWD_DHCSR_DBGKEY 0xA05F0000
#define FD_SWD_DHCSR_STAT_RESET_ST  (1 << 25)
#define FD_SWD_DHCSR_STAT_RETIRE_ST (1 << 24)
#define FD_SWD_DHCSR_STAT_LOCKUP    (1 << 19)
#define FD_SWD_DHCSR_STAT_SLEEP     (1 << 18)
#define FD_SWD_DHCSR_STAT_HALT      (1 << 17)
#define FD_SWD_DHCSR_STAT_REGRDY    (1 << 16)
#define FD_SWD_DHCSR_CTRL_SNAPSTALL (1 << 5)
#define FD_SWD_DHCSR_CTRL_MASKINTS  (1 << 3)
#define FD_SWD_DHCSR_CTRL_STEP      (1 << 2)
#define FD_SWD_DHCSR_CTRL_HALT      (1 << 1)
#define FD_SWD_DHCSR_CTRL_DEBUGEN   (1 << 0)

// Debug Exception and Monitor Control Register definitions
#define FD_SWD_DEMCR_VC_CORERESET   0x00000001  // Reset Vector Catch
#define FD_SWD_DEMCR_VC_MMERR       0x00000010  // Debug Trap on MMU Fault
#define FD_SWD_DEMCR_VC_NOCPERR     0x00000020  // Debug Trap on No Coprocessor Fault
#define FD_SWD_DEMCR_VC_CHKERR      0x00000040  // Debug Trap on Checking Error Fault
#define FD_SWD_DEMCR_VC_STATERR     0x00000080  // Debug Trap on State Error Fault
#define FD_SWD_DEMCR_VC_BUSERR      0x00000100  // Debug Trap on Bus Error Fault
#define FD_SWD_DEMCR_VC_INTERR      0x00000200  // Debug Trap on Interrupt Error Fault
#define FD_SWD_DEMCR_VC_HARDERR     0x00000400  // Debug Trap on Hard Fault
#define FD_SWD_DEMCR_MON_EN         0x00010000  // Monitor Enable
#define FD_SWD_DEMCR_MON_PEND       0x00020000  // Monitor Pend
#define FD_SWD_DEMCR_MON_STEP       0x00040000  // Monitor Step
#define FD_SWD_DEMCR_MON_REQ        0x00080000  // Monitor Request
#define FD_SWD_DEMCR_TRCENA         0x01000000  // Trace Enable (DWT, ITM, ETM, TPIU)

// NVIC: Application Interrupt/Reset Control Register
#define FD_SWD_MEMORY_NVIC_Addr  0xe000e000
#define FD_SWD_MEMORY_NVIC_AIRCR (SWD_MEMORY_NVIC_Addr + 0x0D0C)
#define FD_SWD_MEMORY_NVIC_AIRCR_VECTRESET      0x00000001  // Reset Cortex-M (except Debug)
#define FD_SWD_MEMORY_NVIC_AIRCR_VECTCLRACTIVE  0x00000002  // Clear Active Vector Bit
#define FD_SWD_MEMORY_NVIC_AIRCR_SYSRESETREQ    0x00000004  // Reset System (except Debug)
#define FD_SWD_MEMORY_NVIC_AIRCR_VECTKEY        0x05FA0000  // Write Key

#define FD_SWD_CORTEX_M_REGISTER_R0    0
#define FD_SWD_CORTEX_M_REGISTER_R1    1
#define FD_SWD_CORTEX_M_REGISTER_R2    2
#define FD_SWD_CORTEX_M_REGISTER_R3    3
#define FD_SWD_CORTEX_M_REGISTER_R4    4
#define FD_SWD_CORTEX_M_REGISTER_R5    5
#define FD_SWD_CORTEX_M_REGISTER_R6    6
#define FD_SWD_CORTEX_M_REGISTER_R7    7
#define FD_SWD_CORTEX_M_REGISTER_R8    8
#define FD_SWD_CORTEX_M_REGISTER_R9    9
#define FD_SWD_CORTEX_M_REGISTER_R10  10
#define FD_SWD_CORTEX_M_REGISTER_R11  11
#define FD_SWD_CORTEX_M_REGISTER_R12  12
#define FD_SWD_CORTEX_M_REGISTER_IP   12
#define FD_SWD_CORTEX_M_REGISTER_R13  13
#define FD_SWD_CORTEX_M_REGISTER_SP   13
#define FD_SWD_CORTEX_M_REGISTER_R14  14
#define FD_SWD_CORTEX_M_REGISTER_LR   14
#define FD_SWD_CORTEX_M_REGISTER_R15  15
#define FD_SWD_CORTEX_M_REGISTER_PC   15
#define FD_SWD_CORTEX_M_REGISTER_XPSR 16
#define FD_SWD_CORTEX_M_REGISTER_MSP  17
#define FD_SWD_CORTEX_M_REGISTER_PSP  18

#define FD_SWD_CORTEX_M_REGISTER_S0  0x40
#define FD_SWD_CORTEX_M_REGISTER_S1  0x41
#define FD_SWD_CORTEX_M_REGISTER_S2  0x42
#define FD_SWD_CORTEX_M_REGISTER_S3  0x43
#define FD_SWD_CORTEX_M_REGISTER_S4  0x44
#define FD_SWD_CORTEX_M_REGISTER_S5  0x45
#define FD_SWD_CORTEX_M_REGISTER_S6  0x46
#define FD_SWD_CORTEX_M_REGISTER_S7  0x47
#define FD_SWD_CORTEX_M_REGISTER_S8  0x48
#define FD_SWD_CORTEX_M_REGISTER_S9  0x49
#define FD_SWD_CORTEX_M_REGISTER_S10 0x4a
#define FD_SWD_CORTEX_M_REGISTER_S11 0x4b
#define FD_SWD_CORTEX_M_REGISTER_S12 0x4c
#define FD_SWD_CORTEX_M_REGISTER_S13 0x4d
#define FD_SWD_CORTEX_M_REGISTER_S14 0x4e
#define FD_SWD_CORTEX_M_REGISTER_S15 0x4f
#define FD_SWD_CORTEX_M_REGISTER_S16 0x50
#define FD_SWD_CORTEX_M_REGISTER_S17 0x51
#define FD_SWD_CORTEX_M_REGISTER_S18 0x52
#define FD_SWD_CORTEX_M_REGISTER_S19 0x53
#define FD_SWD_CORTEX_M_REGISTER_S20 0x54
#define FD_SWD_CORTEX_M_REGISTER_S21 0x55
#define FD_SWD_CORTEX_M_REGISTER_S22 0x56
#define FD_SWD_CORTEX_M_REGISTER_S23 0x57
#define FD_SWD_CORTEX_M_REGISTER_S24 0x58
#define FD_SWD_CORTEX_M_REGISTER_S25 0x59
#define FD_SWD_CORTEX_M_REGISTER_S26 0x5a
#define FD_SWD_CORTEX_M_REGISTER_S27 0x5b
#define FD_SWD_CORTEX_M_REGISTER_S28 0x5c
#define FD_SWD_CORTEX_M_REGISTER_S29 0x5d
#define FD_SWD_CORTEX_M_REGISTER_S30 0x5e
#define FD_SWD_CORTEX_M_REGISTER_S31 0x5f

void fd_swd_set_target_id(
    fd_swd_channel_t *channel,
    uint32_t target_id
);

void fd_swd_select_access_port_id(
    fd_swd_channel_t *channel,
    uint8_t access_port_id
);

bool fd_swd_write_data(
    fd_swd_channel_t *channel,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fd_swd_error_t *error
);

bool fd_swd_read_data(
    fd_swd_channel_t *channel,
    uint32_t address,
    uint8_t *data,
    uint32_t length,
    fd_swd_error_t *error
);

bool fd_swd_read_memory_uint32(
    fd_swd_channel_t *channel,
    uint32_t address,
    uint32_t *value,
    fd_swd_error_t *error
);

bool fd_swd_write_memory_uint32(
    fd_swd_channel_t *channel,
    uint32_t address,
    uint32_t value,
    fd_swd_error_t *error
);

typedef enum {
    fd_swd_port_debug,
    fd_swd_port_access,
} fd_swd_port_t;

typedef enum {
    fd_swd_direction_write,
    fd_swd_direction_read,
} fd_swd_direction_t;

typedef enum {
    fd_swd_ack_ok = 0b001,
    fd_swd_ack_wait = 0b010,
    fd_swd_ack_fault = 0b100,
} fd_swd_ack_t;

bool fd_swd_read_port(
    fd_swd_channel_t *channel,
    fd_swd_port_t port,
    uint8_t register_offset,
    uint32_t *value,
    fd_swd_error_t *error
);

bool fd_swd_write_port(
    fd_swd_channel_t *channel,
    fd_swd_port_t port,
    uint8_t register_offset,
    uint32_t value,
    fd_swd_error_t *error
);

bool fd_swd_read_register(
    fd_swd_channel_t *channel,
    uint16_t register_id,
    uint32_t *value,
    fd_swd_error_t *error
);

bool fd_swd_write_register(
    fd_swd_channel_t *channel,
    uint16_t register_id,
    uint32_t value,
    fd_swd_error_t *error
);

void fd_swd_reset_debug_port(
    fd_swd_channel_t *channel
);

bool fd_swd_initialize_debug_port(
    fd_swd_channel_t *channel,
    fd_swd_error_t *error
);

bool fd_swd_read_debug_port(
    fd_swd_channel_t *channel,
    uint8_t register_offset,
    uint32_t *value,
    fd_swd_error_t *error
);

bool fd_swd_write_debug_port(
    fd_swd_channel_t *channel,
    uint8_t register_offset,
    uint32_t value,
    fd_swd_error_t *error
);

bool fd_swd_initialize_access_port(
    fd_swd_channel_t *channel,
    fd_swd_error_t *error
);

bool fd_swd_select_and_read_access_port(
    fd_swd_channel_t *channel,
    uint8_t access_port_register,
    uint32_t *value,
    fd_swd_error_t *error
);

bool fd_swd_select_and_write_access_port(
    fd_swd_channel_t *channel,
    uint8_t access_port_register,
    uint32_t value,
    fd_swd_error_t *error
);

bool fd_swd_is_halted(
    fd_swd_channel_t *channel,
    bool *halted,
    fd_swd_error_t *error
);

bool fd_swd_halt(
    fd_swd_channel_t *channel,
    fd_swd_error_t *error
);

bool fd_swd_step(
    fd_swd_channel_t *channel,
    fd_swd_error_t *error
);

bool fd_swd_run(
    fd_swd_channel_t *channel,
    fd_swd_error_t *error
);

bool fd_swd_connect(
    fd_swd_channel_t *channel,
    uint32_t *dpid,
    fd_swd_error_t *error
);

#endif
