
#ifndef fd_rpc_service_hardware_pb_h
#define fd_rpc_service_hardware_pb_h

#include "fd_rpc.h"

extern const fd_rpc_method_t fd_rpc_service_hardware_get_device_identifier;

extern const fd_rpc_method_t fd_rpc_service_hardware_configure_port;

extern const fd_rpc_method_t fd_rpc_service_hardware_get_port;

extern const fd_rpc_method_t fd_rpc_service_hardware_set_port;

extern const fd_rpc_method_t fd_rpc_service_hardware_spi_transceive;

extern const fd_rpc_method_t fd_rpc_service_hardware_i2c_transfer;

#endif
