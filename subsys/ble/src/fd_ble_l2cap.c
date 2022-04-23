#include "fd_ble_l2cap.h"

#include "fd_assert.h"

#include <bluetooth/l2cap.h>
#include <sys/ring_buffer.h>

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
} fd_ble_l2cap_t;

fd_ble_l2cap_t fd_ble_l2cap;

#define CREDITS 10
#define DATA_MTU (23 * CREDITS)

NET_BUF_POOL_FIXED_DEFINE(fd_ble_l2cap_tx_pool, 1, BT_L2CAP_SDU_BUF_SIZE(DATA_MTU), NULL);
NET_BUF_POOL_FIXED_DEFINE(fd_ble_l2cap_rx_pool, 1, DATA_MTU, NULL);

static struct net_buf *fd_ble_l2cap_chan_ops_alloc_buf(struct bt_l2cap_chan *chan) {
	struct net_buf *buf = net_buf_alloc(fd_ble_l2cap.rx_pool, K_NO_WAIT);
    fd_assert(buf != 0);
    return buf;
}

static int fd_ble_l2cap_chan_ops_recv(struct bt_l2cap_chan *chan, struct net_buf *buf) {
    int rb_len = ring_buf_put(&fd_ble_l2cap.rx_ring_buf, buf->data, buf->len);
    bt_l2cap_chan_recv_complete(&fd_ble_l2cap.le_chan.chan, buf);
    if (rb_len < buf->len) {
        fd_ble_l2cap.rx_drop_count += buf->len - rb_len;
    }
    fd_ble_l2cap.configuration.rx_ready();
	return 0;
}

static void fd_ble_l2cap_chan_ops_sent(struct bt_l2cap_chan *chan) {
}

static void fd_ble_l2cap_chan_ops_status(struct bt_l2cap_chan *chan, atomic_t *status) {
}

static void fd_ble_l2cap_chan_ops_connected(struct bt_l2cap_chan *chan) {
}

static void fd_ble_l2cap_chan_ops_disconnected(struct bt_l2cap_chan *chan) {
}

static int l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan) {
	if (fd_ble_l2cap.le_chan.chan.conn) {
		return -ENOMEM;
	}
	*chan = &fd_ble_l2cap.le_chan.chan;
	return 0;
}

size_t fd_ble_l2cap_get_rx_data(uint8_t *buffer, size_t size) {
    return ring_buf_get(&fd_ble_l2cap.rx_ring_buf, buffer, size);
}

bool fd_ble_l2cap_tx_data(const uint8_t *data, size_t length) {
	struct net_buf *buf = net_buf_alloc(fd_ble_l2cap.tx_pool, K_NO_WAIT);
    fd_assert(buf != 0);
    net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);
    net_buf_add_mem(buf, data, length);
    int ret = bt_l2cap_chan_send(&fd_ble_l2cap.le_chan.chan, buf);
    if (ret < 0) {
        net_buf_unref(buf);
        return false;
    }
	return true;
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
