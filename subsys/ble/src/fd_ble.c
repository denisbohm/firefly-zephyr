#include "fd_ble.h"

#include "fd_assert.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <settings/settings.h>

typedef struct {
    const fd_ble_configuration_t *configuration;
    struct bt_data advertising_data[2];
    struct bt_conn_cb conn_cb;
    struct bt_conn *conn;
    uint8_t disconnect_reason;
} fd_ble_t;

fd_ble_t fd_ble;

void fd_ble_connected(struct bt_conn *conn, uint8_t err) {
	if (err == 0) {
        fd_ble.conn = bt_conn_ref(conn);
	}
}

void fd_ble_disconnected(struct bt_conn *conn, uint8_t reason) {
	fd_ble.disconnect_reason = reason;
    bt_conn_unref(conn);
    fd_ble.conn = 0;
}

void fd_ble_initialize(const fd_ble_configuration_t *configuration) {
    memset(&fd_ble, 0, sizeof(fd_ble));
    fd_ble.configuration = configuration;

    static const uint8_t flags[] = { BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR };
    fd_ble.advertising_data[0] = (struct bt_data) {
        .type = BT_DATA_FLAGS,
        .data = flags,
        .data_len = sizeof(flags),
    };
    fd_ble.advertising_data[1] = (struct bt_data) {
        .type = BT_DATA_UUID128_ALL,
        .data = configuration->custom_service_uuid,
        .data_len = 16,
    };

    fd_ble.conn_cb = (struct bt_conn_cb) {
	    .connected = fd_ble_connected,
	    .disconnected = fd_ble_disconnected,
    };
    bt_conn_cb_register(&fd_ble.conn_cb);

	int err = bt_enable(NULL);
    fd_assert(err == 0);

	settings_load();
}

void fd_ble_start_advertising(void) {
	int err = bt_le_adv_start(
        BT_LE_ADV_CONN_NAME,
        fd_ble.advertising_data,
        ARRAY_SIZE(fd_ble.advertising_data),
        0,
        0
    );
    fd_assert(err == 0);
}