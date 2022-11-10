#include "fd_usb.h"

#include "fd_assert.h"

#include <usb/usb_device.h>

typedef struct {
    uint32_t error;
    uint32_t reset;
    uint32_t connected;
    uint32_t configured;
    uint32_t disconnected;
    uint32_t suspend;
    uint32_t resume;
    uint32_t interface;
    uint32_t set_halt;
    uint32_t clear_halt;
    uint32_t sof;
    uint32_t unknown;
} fd_usb_status_count_t;

typedef struct {
    fd_usb_configuration_t configuration;
    fd_usb_status_count_t status_count;
    bool is_connected;
} fd_usb_t;

fd_usb_t fd_usb;

bool fd_usb_is_connected(void) {
    return fd_usb.is_connected;
}

void fd_usb_dc_status_callback(enum usb_dc_status_code cb_status, const uint8_t *param) {
    switch (cb_status) {
        case USB_DC_ERROR: // USB error reported by the controller
            ++fd_usb.status_count.error;
            break;
        case USB_DC_RESET: // USB reset
            ++fd_usb.status_count.reset;
            break;
        case USB_DC_CONNECTED: // USB connection established, hardware enumeration is completed
            ++fd_usb.status_count.connected;
            fd_usb.is_connected = true;
            if (fd_usb.configuration.connected) {
                fd_usb.configuration.connected();
            }
            break;
        case USB_DC_CONFIGURED: // USB configuration done
            ++fd_usb.status_count.configured;
            break;
        case USB_DC_DISCONNECTED: // USB connection lost
            ++fd_usb.status_count.disconnected;
            fd_usb.is_connected = false;
            if (fd_usb.configuration.disconnected) {
                fd_usb.configuration.disconnected();
            }
            break;
        case USB_DC_SUSPEND: // USB connection suspended by the HOST
            ++fd_usb.status_count.suspend;
            break;
        case USB_DC_RESUME: // USB connection resumed by the HOST
            ++fd_usb.status_count.resume;
            break;
        case USB_DC_INTERFACE: // USB interface selected
            ++fd_usb.status_count.interface;
            break;
        case USB_DC_SET_HALT: // Set Feature ENDPOINT_HALT received
            ++fd_usb.status_count.set_halt;
            break;
        case USB_DC_CLEAR_HALT: // Clear Feature ENDPOINT_HALT received
            ++fd_usb.status_count.clear_halt;
            break;
        case USB_DC_SOF: // Start of Frame received
            ++fd_usb.status_count.sof;
            break;
        case USB_DC_UNKNOWN: // Initial USB connection status
            ++fd_usb.status_count.unknown;
            break;
    }
}

void fd_usb_initialize(const fd_usb_configuration_t *configuration) {
    memset(&fd_usb, 0, sizeof(fd_usb));
    if (configuration) {
        fd_usb.configuration = *configuration;
    }

    int err = usb_enable(fd_usb_dc_status_callback);
    fd_assert(err == 0);
}