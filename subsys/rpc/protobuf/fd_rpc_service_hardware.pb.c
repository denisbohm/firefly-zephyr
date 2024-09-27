
#include "fd_rpc_service_hardware.pb.h"

#include "hardware.pb.h"

const fd_rpc_method_t fd_rpc_service_hardware_get_device_identifier = {
    .package_id = 8,
    .service_id = 1,
    .method_id = 1,
    .request_size = firefly_hardware_v1_GetDeviceIdentifierRequest_size,
    .request_msgdesc = &firefly_hardware_v1_GetDeviceIdentifierRequest_msg,
    .response_size = firefly_hardware_v1_GetDeviceIdentifierResponse_size,
    .response_msgdesc = &firefly_hardware_v1_GetDeviceIdentifierResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_hardware_configure_port = {
    .package_id = 8,
    .service_id = 1,
    .method_id = 2,
    .request_size = firefly_hardware_v1_ConfigurePortRequest_size,
    .request_msgdesc = &firefly_hardware_v1_ConfigurePortRequest_msg,
    .response_size = firefly_hardware_v1_ConfigurePortResponse_size,
    .response_msgdesc = &firefly_hardware_v1_ConfigurePortResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_hardware_get_port = {
    .package_id = 8,
    .service_id = 1,
    .method_id = 3,
    .request_size = firefly_hardware_v1_GetPortRequest_size,
    .request_msgdesc = &firefly_hardware_v1_GetPortRequest_msg,
    .response_size = firefly_hardware_v1_GetPortResponse_size,
    .response_msgdesc = &firefly_hardware_v1_GetPortResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_hardware_set_port = {
    .package_id = 8,
    .service_id = 1,
    .method_id = 4,
    .request_size = firefly_hardware_v1_SetPortRequest_size,
    .request_msgdesc = &firefly_hardware_v1_SetPortRequest_msg,
    .response_size = firefly_hardware_v1_SetPortResponse_size,
    .response_msgdesc = &firefly_hardware_v1_SetPortResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_hardware_spi_transceive = {
    .package_id = 8,
    .service_id = 1,
    .method_id = 5,
    .request_size = firefly_hardware_v1_SpiTransceiveRequest_size,
    .request_msgdesc = &firefly_hardware_v1_SpiTransceiveRequest_msg,
    .response_size = firefly_hardware_v1_SpiTransceiveResponse_size,
    .response_msgdesc = &firefly_hardware_v1_SpiTransceiveResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};

const fd_rpc_method_t fd_rpc_service_hardware_i2c_transfer = {
    .package_id = 8,
    .service_id = 1,
    .method_id = 6,
    .request_size = firefly_hardware_v1_I2cTransferRequest_size,
    .request_msgdesc = &firefly_hardware_v1_I2cTransferRequest_msg,
    .response_size = firefly_hardware_v1_I2cTransferResponse_size,
    .response_msgdesc = &firefly_hardware_v1_I2cTransferResponse_msg,
    .server_streaming = false,
    .client_streaming = false,
    .access = 0,
};
