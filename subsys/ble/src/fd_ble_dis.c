#include "fd_ble_dis.h"

#include <bluetooth/bluetooth.h>
#include <settings/settings.h>

#include <string.h>

void fd_ble_dis_initialize(const fd_ble_dis_configuration_t *configuration) {
    if (configuration->manufacturer) {
	    settings_runtime_set("bt/dis/manuf", configuration->manufacturer, strlen(configuration->manufacturer));        
    }
    if (configuration->model) {
	    settings_runtime_set("bt/dis/model", configuration->model, strlen(configuration->model));
    }
    if (configuration->serial_number) {
	    settings_runtime_set("bt/dis/serial", configuration->serial_number, strlen(configuration->serial_number));
    }
    if (configuration->hardware_revision) {
	    settings_runtime_set("bt/dis/hw", configuration->hardware_revision, strlen(configuration->hardware_revision));
    }
    if (configuration->firmware_revision) {
    	settings_runtime_set("bt/dis/fw", configuration->firmware_revision, strlen(configuration->firmware_revision));
    }
    if (configuration->device_name) {
        bt_set_name(configuration->device_name);
    }
}

