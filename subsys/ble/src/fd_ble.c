#include "fd_ble.h"

#include "fd_assert.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/settings/settings.h>

typedef struct {
    const fd_ble_configuration_t *configuration;
    struct bt_data advertising_data[2];
    struct bt_conn_cb conn_cb;
    struct bt_gatt_cb gatt_cb;
    struct bt_conn *conn;
    uint8_t disconnect_reason;
} fd_ble_t;

fd_ble_t fd_ble;

void fd_ble_le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout) {
    struct bt_conn_info info = {0};
    int result = bt_conn_get_info(conn, &info);
    fd_assert(result == 0);
}

void fd_ble_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx) {
    if (fd_ble.configuration->mtu_updated) {
        uint16_t mtu = bt_gatt_get_mtu(conn);
        fd_ble.configuration->mtu_updated(mtu);
    }
}

void fd_ble_connected(struct bt_conn *conn, uint8_t result) {
	if (result != 0) {
        return;
    }

    fd_ble.conn = bt_conn_ref(conn);

    struct bt_conn_info info = {0};
    result = bt_conn_get_info(conn, &info);
    fd_assert(result == 0);

    struct bt_le_conn_param param = BT_LE_CONN_PARAM_INIT(6 /* 7.5 ms */, 6 /* 30 ms */, 0, 400);
    result = bt_conn_le_param_update(conn, &param);
    fd_assert(result == 0);

    if (fd_ble.configuration->connected) {
        fd_ble.configuration->connected(conn);
    }
}

void fd_ble_disconnected(struct bt_conn *conn, uint8_t reason) {
	fd_ble.disconnect_reason = reason;
    bt_conn_unref(conn);
    fd_ble.conn = 0;

    if (fd_ble.configuration->disconnected) {
        fd_ble.configuration->disconnected(conn);
    }
}

bool fd_ble_is_connected(void) {
    return fd_ble.conn != 0;
}

void *fd_ble_get_connection(void) {
    return fd_ble.conn;
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
        .le_param_updated = fd_ble_le_param_updated,
    };
    bt_conn_cb_register(&fd_ble.conn_cb);

	int result = bt_enable(NULL);
    fd_assert(result == 0);

    fd_ble.gatt_cb = (struct bt_gatt_cb) {
	    .att_mtu_updated = fd_ble_mtu_updated,
    };
	bt_gatt_cb_register(&fd_ble.gatt_cb);

	settings_load();
}

void fd_ble_start_advertising(void) {
	int result = bt_le_adv_start(
        BT_LE_ADV_CONN_NAME,
        fd_ble.advertising_data,
        ARRAY_SIZE(fd_ble.advertising_data),
        0,
        0
    );
    // When the HCI command returns HCI_ERROR_CODE_CMD_DISALLOWED (the device is not in a state to process the command)
    // we only get the return -EIO.  This is happening for the ST BlueNRG-MS after disconnecting for some reason. --denis
    fd_assert((result == 0) || (result == -EALREADY) || (result == -EIO));
}

void fd_ble_stop_advertising(void) {
    int result = bt_le_adv_stop();
    fd_assert(result == 0);
}