#include "fd_ble.h"

#include "fd_assert.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>

#include <stdio.h>

fd_source_push()

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

typedef struct {
    uint64_t git_hash;
    char revision[32];
} fd_ble_soft_device_t;

const static fd_ble_soft_device_t fd_ble_soft_devices[] = {
    {0x2d79a1c86a40b7, "2.9.0_2d79a1c86a40b7"},
    {0xfe2cf96a7f3622, "2.8.0_fe2cf96a7f3622"},
    {0xd6dac7ae08db72, "2.7.0_d6dac7ae08db72"},
    {0x36f0e50e876848, "2.6.0_36f0e50e876848"},
    {0xc593baa9144d8d, "2.5.0_c593baa9144d8d"},
};

const char *fd_ble_lookup_soft_device_revision(uint8_t soft_device_git_hash[7]) {
    uint64_t git_hash = 0;
    for (uint32_t i = 0; i < 7; ++i) {
        git_hash |= (uint64_t)soft_device_git_hash[i] << ((7 - 1 - i) * 8);
    }
    for (uint32_t i = 0; i < ARRAY_SIZE(fd_ble_soft_devices); ++i) {
        const fd_ble_soft_device_t *soft_device = &fd_ble_soft_devices[i];
        if (soft_device->git_hash == git_hash) {
            return soft_device->revision;
        }
    }
    return NULL;
}

void fd_ble_get_version(char *version, size_t size) {
    version[0] = '\0';
    
	struct net_buf *buf = bt_hci_cmd_create(BT_HCI_OP_VS_READ_VERSION_INFO, 0);
    fd_assert(buf != NULL);
	if (!buf) {
		return;
	}

	struct net_buf *rsp = NULL;
	int err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_VERSION_INFO, buf, &rsp);
    fd_assert(err == 0);
	if (err) {
		// uint8_t reason = rsp ? ((struct bt_hci_rp_vs_read_version_info *) rsp->data)->status : 0;
        return;
	}

	struct bt_hci_rp_vs_read_version_info *rp = (void *)rsp->data;
    // nRF Connect SDK Soft Device returns the git hash prefix encoded into the various fields -denis
    // see https://github.com/nrfconnect/sdk-nrfxlib/blob/v2.9.0/softdevice_controller/lib/nrf53/soft-float/manifest.yaml
	uint8_t git_hash[7];
    git_hash[0] = rp->fw_version;
    git_hash[1] = rp->fw_revision & 0xff;
    git_hash[2] = (rp->fw_revision >> 8) & 0xff;
    git_hash[3] = rp->fw_build & 0xff;
    git_hash[4] = (rp->fw_build >> 8) & 0xff;
    git_hash[5] = (rp->fw_build >> 16) & 0xff;
    git_hash[6] = (rp->fw_build >> 24) & 0xff;

	net_buf_unref(rsp);

    const char *revision = fd_ble_lookup_soft_device_revision(git_hash);
    if (revision != NULL) {
        strncpy(version, revision, size);
        return;
    }

    char git_hash_revision[16];
    git_hash_revision[0] = '_';
    for (uint32_t i = 0; i < sizeof(git_hash); ++i) {
        snprintf(&git_hash_revision[1 + i * 2], 3, "%02x", git_hash[i]);
    }
    strncpy(version, git_hash_revision, size);
}

void fd_ble_set_tx_power(uint8_t handle_type, uint16_t handle, int8_t tx_pwr_lvl) {
	struct bt_hci_cp_vs_write_tx_power_level *cp;
	struct net_buf *buf = bt_hci_cmd_create(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL, sizeof(*cp));
    fd_assert(buf != NULL);
	if (!buf) {
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->handle_type = handle_type;
	cp->tx_power_level = tx_pwr_lvl;

	struct net_buf *rsp = NULL;
	int err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_WRITE_TX_POWER_LEVEL, buf, &rsp);
    fd_assert(err == 0);
	if (err) {
		// uint8_t reason = rsp ? ((struct bt_hci_rp_vs_write_tx_power_level *)rsp->data)->status : 0;
		return;
	}

	// struct bt_hci_rp_vs_write_tx_power_level *rp;
	// rp = (void *)rsp->data;
	// printk("Actual Tx Power: %d\n", rp->selected_tx_power);

	net_buf_unref(rsp);
}

void fd_ble_get_tx_power(uint8_t handle_type, uint16_t handle, int8_t *tx_pwr_lvl) {
	*tx_pwr_lvl = 0xFF;
	struct bt_hci_cp_vs_read_tx_power_level *cp;
	struct net_buf *buf = bt_hci_cmd_create(BT_HCI_OP_VS_READ_TX_POWER_LEVEL, sizeof(*cp));
    fd_assert(buf != NULL);
	if (!buf) {
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);
	cp->handle_type = handle_type;

	struct net_buf *rsp = NULL;
	int err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_TX_POWER_LEVEL, buf, &rsp);
    fd_assert(err == 0);
	if (err) {
		// uint8_t reason = rsp ? ((struct bt_hci_rp_vs_read_tx_power_level *) rsp->data)->status : 0;
 		return;
	}

	struct bt_hci_rp_vs_read_tx_power_level *rp = (void *)rsp->data;
	*tx_pwr_lvl = rp->tx_power_level;

	net_buf_unref(rsp);
}

int8_t fd_ble_get_advertising_tx_power(uint32_t id) {
    uint16_t handle = (uint16_t)id;
    int8_t tx_power = 0;
    fd_ble_get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_ADV, handle, &tx_power);
    return tx_power;
}

void fd_ble_set_advertising_tx_power(uint32_t id, int8_t tx_power) {
    uint16_t handle = (uint16_t)id;
    fd_ble_set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_ADV, handle, tx_power);

    int8_t actual_tx_power = 0;
    fd_ble_get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_ADV, handle, &actual_tx_power);
    fd_assert(actual_tx_power == tx_power);
}

int8_t fd_ble_get_connection_tx_power(void *connection) {
    uint16_t handle = 0;
    int result = bt_hci_get_conn_handle((struct bt_conn *)connection, &handle);
	fd_assert(result == 0);

    int8_t tx_power = 0;
    fd_ble_get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN, handle, &tx_power);
    return tx_power;
}

void fd_ble_set_connection_tx_power(void *connection, int8_t tx_power) {
    uint16_t handle = 0;
    int result = bt_hci_get_conn_handle((struct bt_conn *)connection, &handle);
	fd_assert(result == 0);

    fd_ble_set_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN, handle, tx_power);
    int8_t actual_tx_power = 0;
    fd_ble_get_tx_power(BT_HCI_VS_LL_HANDLE_TYPE_CONN, handle, &actual_tx_power);
    fd_assert(actual_tx_power == tx_power);
}

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
    fd_ble.conn = 0;
    bt_conn_unref(conn);

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

fd_source_pop()
