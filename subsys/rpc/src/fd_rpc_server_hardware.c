#include "fd_rpc_server_hardware.h"

#include "fd_assert.h"
#include "fd_rpc.h"
#include "fd_unused.h"

#include "fd_rpc_service_hardware.pb.h"
#include "hardware.pb.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>

fd_source_push()

bool fd_rpc_server_hardware_configure_port_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_hardware_v1_ConfigurePortRequest *request = a_request;
    const struct device *port = device_get_binding(request->port);
    firefly_hardware_v1_ConfigurePortResponse response = {};
    if (port != NULL) {
        for (uint32_t pin = 0; pin < 32; ++pin) {
            if ((request->pins & (1 << pin)) != 0) {
                response.result = gpio_pin_configure(port, (gpio_pin_t)pin, request->flags);
                if (response.result != 0) {
                    break;
                }
            }
        }
    } else {
        response.result = EINVAL;
    }
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_hardware_get_port_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_hardware_v1_GetPortRequest *request = a_request;
    const struct device *port = device_get_binding(request->port);
    firefly_hardware_v1_GetPortResponse response = {};
    if (port != NULL) {
        response.result = gpio_port_get_raw(port, &response.pins);
    } else {
        response.result = EINVAL;
    }
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_hardware_set_port_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_hardware_v1_SetPortRequest *request = a_request;
    const struct device *port = device_get_binding(request->port);
    firefly_hardware_v1_SetPortResponse response = {};
    if (port != NULL) {
        response.result = gpio_port_set_masked_raw(port, request->pins, request->value);
    } else {
        response.result = EINVAL;
    }
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_hardware_get_device_identifier_request(fd_rpc_server_context_t *context, const void *a_request fd_unused) {
    firefly_hardware_v1_GetDeviceIdentifierResponse response = {};
    ssize_t size = hwinfo_get_device_id(response.identifier.bytes, sizeof(response.identifier.bytes));
    if (size > 0) {
        response.identifier.size = (pb_size_t)size;
    }
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_hardware_spi_transceive_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_hardware_v1_SpiTransceiveRequest *request = a_request;
    firefly_hardware_v1_SpiTransceiveResponse response = {};
    const struct device *device = device_get_binding(request->device);
    const struct device *port = device_get_binding(request->chip_select.port);
    if ((device != NULL) && (port != NULL)) {
        const struct spi_config config = {
            .frequency = request->frequency,
            .operation = (spi_operation_t)request->operation,
            .cs = {
                .gpio = {
                    .port = port,
                    .pin = (gpio_pin_t)request->chip_select.pin,
                    .dt_flags = (gpio_dt_flags_t)request->chip_select.flags,
                }
            }
        };
        const struct spi_buf tx_bufs[] = {
            {
                .buf = (uint8_t *)request->data.bytes,
                .len = request->data.size,
            }
        };
        const struct spi_buf_set tx = {
            .buffers = tx_bufs,
            .count = ARRAY_SIZE(tx_bufs),
        };
        const struct spi_buf rx_bufs[] = {
            {
                .buf = (uint8_t *)response.data.bytes,
                .len = request->data.size,
            }
        };
        const struct spi_buf_set rx = {
            .buffers = rx_bufs,
            .count = ARRAY_SIZE(rx_bufs),
        };
        response.result = spi_transceive(device, &config, &tx, &rx);
    } else {
        response.result = ENXIO;
    }
    return fd_rpc_server_send_client_response(context, &response);
}

bool fd_rpc_server_hardware_i2c_transfer_request(fd_rpc_server_context_t *context, const void *a_request) {
    const firefly_hardware_v1_I2cTransferRequest *request = a_request;
    firefly_hardware_v1_I2cTransferResponse response = {};
    const struct device *device = device_get_binding(request->device);
    if (device != NULL) {
        uint32_t speed = I2C_SPEED_DT;
        if (request->frequency == 100000) {
            speed = I2C_SPEED_STANDARD;
        } else
        if (request->frequency == 400000) {
            speed = I2C_SPEED_FAST;
        } else
        if (request->frequency == 1000000) {
            speed = I2C_SPEED_FAST_PLUS;
        } else
        if (request->frequency == 3400000) {
            speed = I2C_SPEED_HIGH;
        } else
        if (request->frequency == 5000000) {
            speed = I2C_SPEED_ULTRA;
        }
        uint32_t dev_config = I2C_MODE_CONTROLLER | I2C_SPEED_SET(speed);
        struct i2c_msg msgs[ARRAY_SIZE(request->operations)];
        for (uint32_t i = 0; i < request->operations_count; ++i) {
            const firefly_hardware_v1_I2cOperation *operation = &request->operations[i];
            if (operation->which_msg == firefly_hardware_v1_I2cOperation_read_tag) {
                struct i2c_msg *msg = &msgs[i];
                firefly_hardware_v1_I2cResult *result = &response.results[response.results_count];
                msg->buf = result->msg.read.data.bytes;
                msg->len = operation->msg.read.length;
                msg->flags = (uint8_t)operation->msg.read.flags;
                result->which_msg = firefly_hardware_v1_I2cResult_read_tag;
                result->msg.read.data.size = (pb_size_t)operation->msg.read.length;
                ++response.results_count;
            } else
            if (operation->which_msg == firefly_hardware_v1_I2cOperation_write_tag) {
                struct i2c_msg *msg = &msgs[i];
                msg->buf = (uint8_t *)operation->msg.write.data.bytes;
                msg->len = operation->msg.write.data.size;
                msg->flags = (uint8_t)operation->msg.write.flags;
            }
        }
        int result = i2c_configure(device, dev_config);
        if (result == 0) {
            response.result = i2c_transfer(device, msgs, (uint8_t)request->operations_count, (uint16_t)request->address);
        } else {
            response.result = result;
        }
    } else {
        response.result = ENXIO;
    }
    return fd_rpc_server_send_client_response(context, &response);
}

void fd_rpc_server_hardware_initialize(void) {
    fd_rpc_set_method_server(fd, hardware, get_device_identifier);

    fd_rpc_set_method_server(fd, hardware, configure_port);
    fd_rpc_set_method_server(fd, hardware, get_port);
    fd_rpc_set_method_server(fd, hardware, set_port);

    fd_rpc_set_method_server(fd, hardware, spi_transceive);
    fd_rpc_set_method_server(fd, hardware, i2c_transfer);
}

fd_source_pop()
