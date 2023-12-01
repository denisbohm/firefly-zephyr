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
    uint8_t service_uuid[16];
    struct bt_data advertising_data[2];
    struct bt_data scan_response_data[1];
    struct bt_conn_cb conn_cb;
    struct bt_gatt_cb gatt_cb;
    struct bt_conn *conn;
    struct bt_conn_info conn_info;
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
    struct bt_conn_le_data_len_info conn_le_data_len_info;
#endif
    uint8_t disconnect_reason;
} fd_ble_t;

fd_ble_t fd_ble;

void fd_ble_le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout) {
    int result = bt_conn_get_info(conn, &fd_ble.conn_info);
    fd_assert(result == 0);
}

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
void fd_ble_le_data_len_updated(struct bt_conn *conn, struct bt_conn_le_data_len_info *info) {
    fd_ble.conn_le_data_len_info = *info;
}
#endif

void fd_ble_mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx) {
    if (fd_ble.configuration->mtu_updated) {
        uint16_t mtu = bt_gatt_get_mtu(conn);
        fd_ble.configuration->mtu_updated(mtu);
    }
}

uint8_t fd_ble_get_disconnect_reason(void) {
    return fd_ble.disconnect_reason;
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

#if 1
    struct bt_conn_le_data_len_param len_param = {
        .tx_max_len = 251,
        .tx_max_time = 2120,
    };
    result = bt_conn_le_data_len_update(conn, &len_param);
    fd_assert(result == 0);
#endif

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

bool fd_ble_initialize(const fd_ble_configuration_t *configuration) {
    memset(&fd_ble, 0, sizeof(fd_ble));
    fd_ble.configuration = configuration;
    if (configuration->custom_service_uuid != NULL) {
        memcpy(fd_ble.service_uuid, configuration->custom_service_uuid, sizeof(fd_ble.service_uuid));
    }

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

    fd_ble.scan_response_data[0] = (struct bt_data) {
        .type = BT_DATA_MANUFACTURER_DATA,
        .data = configuration->manufacturer_data,
        .data_len = configuration->manufacturer_data_size,
    };

    fd_ble.conn_cb = (struct bt_conn_cb) {
	    .connected = fd_ble_connected,
	    .disconnected = fd_ble_disconnected,
        .le_param_updated = fd_ble_le_param_updated,
#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
        .le_data_len_updated = fd_ble_le_data_len_updated,
#endif
    };
    bt_conn_cb_register(&fd_ble.conn_cb);

	int result = bt_enable(NULL);
    fd_assert(result == 0);
    if (result != 0) {
        return false;
    }

    fd_ble.gatt_cb = (struct bt_gatt_cb) {
	    .att_mtu_updated = fd_ble_mtu_updated,
    };
	bt_gatt_cb_register(&fd_ble.gatt_cb);

	settings_load();

    return true;
}

void fd_ble_set_service_uuid(const uint8_t uuid[16]) {
    memcpy(fd_ble.service_uuid, uuid, sizeof(fd_ble.service_uuid));
    fd_ble.advertising_data[1].data = fd_ble.service_uuid;
}

void fd_ble_set_manufacturer_data(const uint8_t *data, size_t size) {
    fd_ble.scan_response_data[0] = (struct bt_data) {
        .type = BT_DATA_MANUFACTURER_DATA,
        .data = data,
        .data_len = size,
    };
}

void fd_ble_start_advertising_with_id(uint32_t id) {
    int result = bt_le_adv_stop();
    fd_assert(result >= 0);
    struct bt_le_adv_param adv_param = {
        .id = id,
        .sid = 0,
        .secondary_max_skip = 0,
        .options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
        .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
        .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
        .peer = NULL,
    };
	result = bt_le_adv_start(
        &adv_param,
        fd_ble.advertising_data,
        ARRAY_SIZE(fd_ble.advertising_data),
        fd_ble.scan_response_data,
        fd_ble.scan_response_data[0].data_len > 0 ? ARRAY_SIZE(fd_ble.scan_response_data) : 0
    );
    // When the HCI command returns HCI_ERROR_CODE_CMD_DISALLOWED (the device is not in a state to process the command)
    // we only get the return -EIO.  This is happening for the ST BlueNRG-MS after disconnecting for some reason. --denis
    fd_assert((result == 0) || (result == -EALREADY) || (result == -EIO));
}

void fd_ble_start_advertising(void) {
    fd_ble_start_advertising_with_id(BT_ID_DEFAULT);
}

void fd_ble_disconnect(void) {
    if (fd_ble.conn == NULL) {
        return;
    }
    int result = bt_conn_disconnect(fd_ble.conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    fd_assert(result >= 0);
}

void fd_ble_stop_advertising(void) {
    int result = bt_le_adv_stop();
    fd_assert(result == 0);
}