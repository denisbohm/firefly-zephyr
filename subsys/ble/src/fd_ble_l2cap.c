#include "fd_ble_l2cap.h"

#include "fd_assert.h"
#include "fd_unused.h"

#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/sys/ring_buffer.h>
#include <version.h>

#include <string.h>

typedef struct {
    fd_ble_l2cap_configuration_t configuration;
    struct bt_l2cap_server server;
    struct bt_l2cap_le_chan le_chan;
    struct bt_l2cap_chan_ops chan_ops;
    struct net_buf_pool *tx_pool;
    struct net_buf_pool *rx_pool;
    uint8_t rx_ring_buffer[1024];
    struct ring_buf rx_ring_buf;
    uint32_t rx_count;
    uint32_t rx_drop_count;
    uint8_t tx_ring_buffer[8192];
    struct ring_buf tx_ring_buf;
    uint32_t tx_count;
    uint32_t tx_alloc_count;
    uint32_t tx_emsgsize_count;
    uint32_t tx_enobufs_count;
    uint32_t tx_eother_count;
    struct k_work recv_work;
    struct k_work send_work;
    struct k_work sent_work;
    struct k_work connected_work;
    struct k_work disconnected_work;
    bool connected;
} fd_ble_l2cap_t;

fd_ble_l2cap_t fd_ble_l2cap;

#define CREDITS 10
#define DATA_MTU (23 * CREDITS)

#if KERNEL_VERSION_MAJOR >= 3
NET_BUF_POOL_FIXED_DEFINE(fd_ble_l2cap_tx_pool, 1, BT_L2CAP_SDU_BUF_SIZE(DATA_MTU), 8, NULL);
NET_BUF_POOL_FIXED_DEFINE(fd_ble_l2cap_rx_pool, 1, DATA_MTU, 8, NULL);
#else
NET_BUF_POOL_FIXED_DEFINE(fd_ble_l2cap_tx_pool, 1, BT_L2CAP_SDU_BUF_SIZE(DATA_MTU), NULL);
NET_BUF_POOL_FIXED_DEFINE(fd_ble_l2cap_rx_pool, 1, DATA_MTU, NULL);
#endif

static struct net_buf *fd_ble_l2cap_chan_ops_alloc_buf(struct bt_l2cap_chan *chan) {
	struct net_buf *buf = net_buf_alloc(fd_ble_l2cap.rx_pool, K_NO_WAIT);
    fd_assert(buf != 0);
    return buf;
}

void fd_ble_l2cap_recv_work(struct k_work *work fd_unused) {
    while (true) {
        uint8_t data[64];
        uint32_t length = ring_buf_get(&fd_ble_l2cap.rx_ring_buf, data, sizeof(data));
        if (length <= 0) {
            break;
        }
        if (fd_ble_l2cap.configuration.recv) {
            fd_ble_l2cap.configuration.recv(data, length);
        }
    }
}

static int fd_ble_l2cap_chan_ops_recv(struct bt_l2cap_chan *chan, struct net_buf *buf) {
    int rb_len = ring_buf_put(&fd_ble_l2cap.rx_ring_buf, buf->data, buf->len);
    bt_l2cap_chan_recv_complete(&fd_ble_l2cap.le_chan.chan, buf);
    if (rb_len < buf->len) {
        fd_ble_l2cap.rx_drop_count += buf->len - rb_len;
    }

    int result = k_work_submit_to_queue(fd_ble_l2cap.configuration.work_queue, &fd_ble_l2cap.recv_work);
    fd_assert(result >= 0);
	return 0;
}

static void fd_ble_l2cap_chan_ops_sent(struct bt_l2cap_chan *chan) {
    k_work_submit_to_queue(fd_ble_l2cap.configuration.work_queue, &fd_ble_l2cap.sent_work);
}

static void fd_ble_l2cap_chan_ops_status(struct bt_l2cap_chan *chan, atomic_t *status) {
}

void fd_ble_l2cap_connected_work(struct k_work *context fd_unused) {
    if (fd_ble_l2cap.configuration.connected) {
        fd_ble_l2cap.configuration.connected();
    }
}

static void fd_ble_l2cap_chan_ops_connected(struct bt_l2cap_chan *chan) {
    fd_ble_l2cap.connected = true;
    k_work_submit_to_queue(fd_ble_l2cap.configuration.work_queue, &fd_ble_l2cap.connected_work);
}

void fd_ble_l2cap_disconnected_work(struct k_work *context fd_unused) {
    if (fd_ble_l2cap.configuration.disconnected) {
        fd_ble_l2cap.configuration.disconnected();
    }
}

static void fd_ble_l2cap_chan_ops_disconnected(struct bt_l2cap_chan *chan) {
    fd_ble_l2cap.connected = false;
    ring_buf_reset(&fd_ble_l2cap.rx_ring_buf);
    ring_buf_reset(&fd_ble_l2cap.tx_ring_buf);

    k_work_submit_to_queue(fd_ble_l2cap.configuration.work_queue, &fd_ble_l2cap.disconnected_work);
}

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_server *server, struct bt_l2cap_chan **chan) {
	if (fd_ble_l2cap.le_chan.chan.conn) {
		return -ENOMEM;
	}
	*chan = &fd_ble_l2cap.le_chan.chan;
	return 0;
}

size_t fd_ble_l2cap_tx(const uint8_t *data, size_t length) {
    struct net_buf *buf = net_buf_alloc(fd_ble_l2cap.tx_pool, K_NO_WAIT);
    if (buf == NULL) {
        return 0;
    }
    ++fd_ble_l2cap.tx_alloc_count;
    net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);
    net_buf_add_mem(buf, data, length);
    int result = bt_l2cap_chan_send(&fd_ble_l2cap.le_chan.chan, buf);
    if (result < 0) {
        if (result == -EMSGSIZE) {
            ++fd_ble_l2cap.tx_emsgsize_count;
        } else
        if (result == -ENOBUFS) {
            ++fd_ble_l2cap.tx_enobufs_count;
        } else {
            ++fd_ble_l2cap.tx_eother_count;
        }
        net_buf_unref(buf);
        return 0;
    }

    // !!! There is a bug in zephyr where it does not return the number of bytes sent,
    // as the documentation states bt_l2cap_chan_send should.
    // Zephyr now fixed in latest (post 3.3.0).
    // So for now assume all sent. -denis
    result = length;

    fd_ble_l2cap.tx_count += result;
    return result;
}

void fd_ble_l2cap_tx_check(void) {
    if (fd_ble_l2cap.le_chan.chan.conn == NULL) {
        return;
    }
    size_t limit = fd_ble_l2cap.le_chan.tx.mtu;
    if (CONFIG_PISON_SUBSYS_FOUNDATION_BLE_TX_LIMIT < limit) {
        limit = CONFIG_PISON_SUBSYS_FOUNDATION_BLE_TX_LIMIT;
    }
    limit -= BT_L2CAP_SDU_CHAN_SEND_RESERVE;
    bool sent = false;
    while (true) {
        uint8_t *data = NULL;
        uint32_t length = ring_buf_get_claim(&fd_ble_l2cap.tx_ring_buf, &data, limit);
        if (length == 0) {
            break;
        }
        size_t amount = fd_ble_l2cap_tx(data, length);
        ring_buf_get_finish(&fd_ble_l2cap.tx_ring_buf, amount);
        if (amount == 0) {
            break;
        }
        sent = true;
    }
    if (sent) {
        k_work_submit_to_queue(fd_ble_l2cap.configuration.work_queue, &fd_ble_l2cap.send_work);
        return;
    }
}

bool fd_ble_l2cap_send(const uint8_t *data, size_t size) {
    if (!fd_ble_l2cap.connected) {
        return false;
    }
    uint32_t space = ring_buf_space_get(&fd_ble_l2cap.tx_ring_buf);
    if (size > space) {
        return false;
    }
    uint32_t length = ring_buf_put(&fd_ble_l2cap.tx_ring_buf, data, size);
    fd_assert(length == size);

    fd_ble_l2cap_tx_check();

    return true;
}

void fd_ble_l2cap_sent_work(struct k_work *context fd_unused) {
    fd_ble_l2cap_tx_check();
}

void fd_ble_l2cap_send_work(struct k_work *context fd_unused) {
    if (fd_ble_l2cap.configuration.sent) {
        fd_ble_l2cap.configuration.sent();
    }
}

uint16_t fd_ble_l2cap_get_psm(void) {
    return fd_ble_l2cap.server.psm;
}

void fd_ble_l2cap_initialize(fd_ble_l2cap_configuration_t configuration) {
    memset(&fd_ble_l2cap, 0, sizeof(fd_ble_l2cap));

    fd_ble_l2cap.configuration = configuration;

    fd_ble_l2cap.tx_pool = &fd_ble_l2cap_tx_pool;
    fd_ble_l2cap.rx_pool = &fd_ble_l2cap_rx_pool;

    ring_buf_init(&fd_ble_l2cap.rx_ring_buf, sizeof(fd_ble_l2cap.rx_ring_buffer), fd_ble_l2cap.rx_ring_buffer);

    k_work_init(&fd_ble_l2cap.recv_work, fd_ble_l2cap_recv_work);
    k_work_init(&fd_ble_l2cap.sent_work, fd_ble_l2cap_sent_work);
    k_work_init(&fd_ble_l2cap.send_work, fd_ble_l2cap_send_work);
    k_work_init(&fd_ble_l2cap.connected_work, fd_ble_l2cap_connected_work);
    k_work_init(&fd_ble_l2cap.disconnected_work, fd_ble_l2cap_disconnected_work);

    fd_ble_l2cap.server = (struct bt_l2cap_server) {
        .psm = 0,
        .sec_level = BT_SECURITY_L1,
        .accept = l2cap_accept,
    };

    int result = bt_l2cap_server_register(&fd_ble_l2cap.server);
    fd_assert(result == 0);

    fd_ble_l2cap.chan_ops = (struct bt_l2cap_chan_ops) {
        .alloc_buf	  = fd_ble_l2cap_chan_ops_alloc_buf,
        .recv		  = fd_ble_l2cap_chan_ops_recv,
        .sent		  = fd_ble_l2cap_chan_ops_sent,
        .status		  = fd_ble_l2cap_chan_ops_status,
        .connected	  = fd_ble_l2cap_chan_ops_connected,
        .disconnected = fd_ble_l2cap_chan_ops_disconnected,
    };
    fd_ble_l2cap.le_chan.chan.ops = &fd_ble_l2cap.chan_ops;

    fd_ble_l2cap.le_chan.rx.mtu = DATA_MTU;
}
