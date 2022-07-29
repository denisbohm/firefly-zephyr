#ifndef fd_ble_dis_h
#define fd_ble_dis_h

typedef struct {
    const char *manufacturer;
    const char *model;
    const char *device_name;
    const char *serial_number;
    const char *hardware_revision;
    const char *firmware_revision;
} fd_ble_dis_configuration_t;

void fd_ble_dis_initialize(const fd_ble_dis_configuration_t *configuration);

#endif