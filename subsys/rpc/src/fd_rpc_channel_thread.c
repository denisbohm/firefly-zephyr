#include "fd_rpc_channel_thread.h"

#include "fd_assert.h"
#include "fd_binary.h"
#include "fd_rpc.h"
#include "fd_rpc_stream.h"
#include "fd_unused.h"
#include "fd_usb.h"

#include <openthread/dns_client.h>
#include <openthread/instance.h>
#include <openthread/message.h>
#include <openthread/srp_client.h>
#include <openthread/tcp.h>
#include <openthread/thread.h>
#include <openthread/thread_ftd.h>
#include <openthread/udp.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/ring_buffer.h>

#include <stdio.h>
#include <string.h>

fd_source_push()

typedef struct {
    otTcpEndpoint endpoint;
    otTcpListener listener;
} fd_rpc_channel_thread_ot_tcp_t;

typedef struct {
    k_timeout_t send_delay;
    otUdpSocket socket;
    otMessageInfo message_info;
    struct k_work_delayable send_done_work;
    fd_rpc_stream_t stream;
} fd_rpc_channel_thread_ot_udp_t;

typedef struct {
    void (*up)(void);
    void (*down)(void);
    otError (*send)(void);
} fd_rpc_channel_thread_protocol_t;

typedef struct {
    otInstance *instance;
    otDeviceRole role;

    fd_rpc_channel_thread_ot_tcp_t tcp;
    fd_rpc_channel_thread_ot_udp_t udp;
    fd_rpc_channel_thread_protocol_t protocol;

    uint8_t receive_buffer[CONFIG_FIREFLY_SUBSYS_RPC_THREAD_RECEIVE_BUFFER_SIZE];
    uint8_t send_buffer[CONFIG_FIREFLY_SUBSYS_RPC_THREAD_SEND_BUFFER_SIZE];
    otLinkedBuffer send_linked_buffer;
    volatile bool is_send_pending;
    int64_t send_time;
    int64_t max_send_time;

    char host_name[128];
    char service_name[128];
    char service_instance_name[128];
    otSrpClientService service;
    otError service_error;
} fd_rpc_channel_thread_ot_t;

typedef struct {
    fd_rpc_channel_thread_configuration_t configuration;
    const fd_rpc_channel_thread_listener_t *listeners[4];
    uint32_t listener_count;

    fd_rpc_channel_thread_state_t state;

    fd_rpc_channel_t channel;
    fd_rpc_channel_free_space_increased_callback_t free_space_increased_callback;
    struct k_work connected_work;
    struct k_work disconnected_work;
    struct k_work rx_work;
    struct k_work tx_work;
    struct k_timer timer;

    uint8_t rx_ring_buffer[CONFIG_FIREFLY_SUBSYS_RPC_THREAD_RX_BUFFER_SIZE];
    struct ring_buf rx_ringbuf;
    uint8_t tx_ring_buffer[CONFIG_FIREFLY_SUBSYS_RPC_THREAD_TX_BUFFER_SIZE];
    struct ring_buf tx_ringbuf;
    uint8_t rx_packet_buffer[CONFIG_FIREFLY_SUBSYS_RPC_BUFFER_SIZE];
    fd_binary_t rx_packet;

    uint32_t tx_ring_count;
    size_t tx_ring_bytes;
    uint32_t tx_thread_count;
    size_t tx_thread_bytes;

    bool is_connected;

    fd_rpc_channel_thread_ot_t ot;
} fd_rpc_channel_thread_t;

fd_rpc_channel_thread_t fd_rpc_channel_thread;

const size_t fd_rpc_channel_thread_low_water_mark = sizeof(fd_rpc_channel_thread.tx_ring_buffer) / 2;

otInstance *fd_rpc_channel_thread_get_ot_instance(void) {
    return fd_rpc_channel_thread.ot.instance;
}

fd_rpc_channel_thread_status_t fd_rpc_channel_thread_get_status(void) {
    return (fd_rpc_channel_thread_status_t) {
        .state = fd_rpc_channel_thread.state,
        .role = fd_rpc_channel_thread.ot.role,
    };
}

void fd_rpc_channel_thread_set_state(fd_rpc_channel_thread_state_t state) {
    fd_rpc_channel_thread.state = state;

    for (uint32_t i = 0; i < fd_rpc_channel_thread.listener_count; ++i) {
        const fd_rpc_channel_thread_listener_t *listener = fd_rpc_channel_thread.listeners[i];
        if (listener->status_changed != NULL) {
            listener->status_changed(fd_rpc_channel_thread_get_status());
        }
    }
}

void fd_rpc_channel_thread_reset_buffers(void) {
	ring_buf_init(&fd_rpc_channel_thread.rx_ringbuf, sizeof(fd_rpc_channel_thread.rx_ring_buffer), fd_rpc_channel_thread.rx_ring_buffer);
	ring_buf_init(&fd_rpc_channel_thread.tx_ringbuf, sizeof(fd_rpc_channel_thread.tx_ring_buffer), fd_rpc_channel_thread.tx_ring_buffer);
    fd_rpc_channel_thread.ot.is_send_pending = false;
    fd_binary_reset(&fd_rpc_channel_thread.rx_packet);
}

void fd_rpc_channel_thread_rx_work(struct k_work *context fd_unused) {
    while (true) {
        uint8_t data[64];
        size_t length = ring_buf_get(&fd_rpc_channel_thread.rx_ringbuf, data, sizeof(data));
        if (length <= 0) {
            break;
        }

        bool result = fd_rpc_channel_received_data(&fd_rpc_channel_thread.channel, &fd_rpc_channel_thread.rx_packet, data, length);
        fd_assert(result);
    }
}

void fd_rpc_channel_thread_rx(const uint8_t *data, size_t length) {
    uint32_t space = ring_buf_space_get(&fd_rpc_channel_thread.rx_ringbuf);
    fd_assert(space >= length);
    int rb_length = ring_buf_put(&fd_rpc_channel_thread.rx_ringbuf, data, length);
    fd_assert(rb_length == length);
    if (rb_length < length) {
        // Dropped length - rb_length bytes
    }
    int result = k_work_submit_to_queue(fd_rpc_channel_thread.configuration.work_queue, &fd_rpc_channel_thread.rx_work);
    fd_assert(result >= 0);
}

void fd_rpc_channel_thread_connected_work(struct k_work *work fd_unused) {
    fd_rpc_channel_opened(&fd_rpc_channel_thread.channel);

    fd_rpc_channel_thread_set_state(fd_rpc_channel_thread_state_connected);

    k_timer_start(&fd_rpc_channel_thread.timer, K_MSEC(250), K_MSEC(250));

    for (uint32_t i = 0; i < fd_rpc_channel_thread.listener_count; ++i) {
        const fd_rpc_channel_thread_listener_t *listener = fd_rpc_channel_thread.listeners[i];
        if (listener->connected != NULL) {
            listener->connected();
        }
    }
}

void fd_rpc_channel_thread_connected(void) {
    fd_rpc_channel_thread_reset_buffers();

    fd_rpc_channel_thread.is_connected = true;
    k_work_submit_to_queue(fd_rpc_channel_thread.configuration.work_queue, &fd_rpc_channel_thread.connected_work);
}

void fd_rpc_channel_thread_disconnected_work(struct k_work *work fd_unused) {
    k_timer_stop(&fd_rpc_channel_thread.timer);

    fd_rpc_channel_closed(&fd_rpc_channel_thread.channel);

    if (fd_rpc_channel_thread.state == fd_rpc_channel_thread_state_connected) {
        fd_rpc_channel_thread_set_state(fd_rpc_channel_thread_state_listening);
    }

    for (uint32_t i = 0; i < fd_rpc_channel_thread.listener_count; ++i) {
        const fd_rpc_channel_thread_listener_t *listener = fd_rpc_channel_thread.listeners[i];
        if (listener->disconnected != NULL) {
            listener->disconnected();
        }
    }
}

void fd_rpc_channel_thread_disconnected(void) {
    fd_rpc_channel_thread.is_connected = false;

    fd_rpc_channel_thread_reset_buffers();

    k_work_submit_to_queue(fd_rpc_channel_thread.configuration.work_queue, &fd_rpc_channel_thread.disconnected_work);
}   

void fd_rpc_channel_thread_send_done(void) {
    fd_assert(fd_rpc_channel_thread.ot.is_send_pending);
    fd_rpc_channel_thread.ot.is_send_pending = false;
    int64_t now = k_uptime_get();
    int64_t delta = now - fd_rpc_channel_thread.ot.send_time;
    if (delta > fd_rpc_channel_thread.ot.max_send_time) {
        fd_rpc_channel_thread.ot.max_send_time = delta;
    }
    // Can happen if device goes out of range. -denis
    // fd_assert(delta < 5000);
    k_work_submit_to_queue(fd_rpc_channel_thread.configuration.work_queue, &fd_rpc_channel_thread.tx_work);
}

otError fd_rpc_channel_thread_tcp_send(void) {
    const uint32_t flags = 0;
    otError error fd_unused = otTcpSendByReference(&fd_rpc_channel_thread.ot.tcp.endpoint, &fd_rpc_channel_thread.ot.send_linked_buffer, flags);
    // Can get an error when socket is closed before this work item is executed. -denis
    // fd_assert(error == OT_ERROR_NONE);
    return error;
}

void fd_rpc_channel_thread_udp_send_done_work(struct k_work *work fd_unused) {
    fd_rpc_channel_thread_send_done();
}

otError fd_rpc_channel_thread_udp_send(void) {
    bool result = fd_rpc_stream_send_data(&fd_rpc_channel_thread.ot.udp.stream, fd_rpc_channel_thread.ot.send_linked_buffer.mData, fd_rpc_channel_thread.ot.send_linked_buffer.mLength);
    if (!result) {
        return OT_ERROR_NO_BUFS;
    }
    return OT_ERROR_NONE;
}

void fd_rpc_channel_thread_tx(void) {
    if (fd_rpc_channel_thread.ot.is_send_pending) {
        return;
    }

    uint8_t *buffer = NULL;
    uint32_t length = ring_buf_get_claim(&fd_rpc_channel_thread.tx_ringbuf, &buffer, sizeof(fd_rpc_channel_thread.ot.send_buffer));
    if (length == 0) {
        int result = ring_buf_get_finish(&fd_rpc_channel_thread.tx_ringbuf, 0);
        fd_assert(result == 0);
        return;
    }

    fd_rpc_channel_thread.ot.send_time = k_uptime_get();
    fd_rpc_channel_thread.ot.is_send_pending = true;
    memcpy(fd_rpc_channel_thread.ot.send_buffer, buffer, length);
    fd_rpc_channel_thread.ot.send_linked_buffer.mData = fd_rpc_channel_thread.ot.send_buffer;
    fd_rpc_channel_thread.ot.send_linked_buffer.mLength = length;

    otError error = fd_rpc_channel_thread.ot.protocol.send();
    if (error != OT_ERROR_NONE) {
        fd_rpc_channel_thread.ot.is_send_pending = false;

        int result = ring_buf_get_finish(&fd_rpc_channel_thread.tx_ringbuf, 0);
        fd_assert(result == 0);
        return;
    }

    int result = ring_buf_get_finish(&fd_rpc_channel_thread.tx_ringbuf, length);
    fd_assert(result == 0);

    ++fd_rpc_channel_thread.tx_thread_count;
    fd_rpc_channel_thread.tx_thread_bytes += length;

    if (fd_rpc_channel_thread.free_space_increased_callback != NULL) {
        fd_rpc_channel_thread.free_space_increased_callback(&fd_rpc_channel_thread.channel);
    }
}

void fd_rpc_channel_thread_tx_work(struct k_work *work fd_unused) {
    fd_rpc_channel_thread_tx();
}

bool fd_rpc_channel_thread_packet_write(const uint8_t *data, size_t size) {
    uint32_t space = ring_buf_space_get(&fd_rpc_channel_thread.tx_ringbuf);
    if (size > space) {
        return false;
    }
    uint32_t length = ring_buf_put(&fd_rpc_channel_thread.tx_ringbuf, data, size);
    fd_assert(length == size);
    ++fd_rpc_channel_thread.tx_ring_count;
    fd_rpc_channel_thread.tx_ring_bytes += length;

    int result = k_work_submit_to_queue(fd_rpc_channel_thread.configuration.work_queue, &fd_rpc_channel_thread.tx_work);
    fd_assert(result >= 0);
    return true;
}

void fd_rpc_channel_thread_timer(struct k_timer *timer fd_unused) {
    fd_rpc_channel_thread_tx();
}

size_t fd_rpc_channel_thread_get_free_space(void) {
    uint32_t space = ring_buf_space_get(&fd_rpc_channel_thread.tx_ringbuf);
    return space;
}

size_t fd_rpc_channel_thread_get_rx_free_space(void) {
    uint32_t space = ring_buf_space_get(&fd_rpc_channel_thread.rx_ringbuf);
    return space;
}

void fd_rpc_channel_thread_set_free_space_increased_callback(fd_rpc_channel_free_space_increased_callback_t callback) {
    fd_rpc_channel_thread.free_space_increased_callback = callback;
}

void fd_rpc_channel_thread_add_listener(const fd_rpc_channel_thread_listener_t *listener) {
    fd_assert(fd_rpc_channel_thread.listener_count < ARRAY_SIZE(fd_rpc_channel_thread.listeners));
    fd_rpc_channel_thread.listeners[fd_rpc_channel_thread.listener_count++] = listener;
}

void fd_rpc_channel_thread_ot_SrpClientCallback(
    otError aError,
    const otSrpClientHostInfo *aHostInfo,
    const otSrpClientService *aServices,
    const otSrpClientService *aRemovedServices,
    void *aContext
) {
    for (const otSrpClientService *service = aServices; service != NULL; service = service->mNext) {
        if (service == &fd_rpc_channel_thread.ot.service) {
            if (aError != OT_ERROR_NONE) {
                fd_rpc_channel_thread.ot.service_error = aError;
            }
            break;
        }
    }
}

void fd_rpc_channel_thread_ot_SrpClientAutoStartCallback(const otSockAddr *aServerSockAddr, void *aContext) {
    // Is called with NULL when stopping. -denis
    // fd_assert(aServerSockAddr != NULL);
}

otTcpIncomingConnectionAction fd_rpc_channel_thread_ot_TcpAcceptReady(otTcpListener *aListener, const otSockAddr *aPeer, otTcpEndpoint **aAcceptInto) {
    *aAcceptInto = &fd_rpc_channel_thread.ot.tcp.endpoint;
    return OT_TCP_INCOMING_CONNECTION_ACTION_ACCEPT;
}

void fd_rpc_channel_thread_ot_TcpAcceptDone(otTcpListener *aListener, otTcpEndpoint *aEndpoint, const otSockAddr *aPeer) {
	fd_rpc_channel_thread_connected();
}

void fd_rpc_channel_thread_ot_TcpEstablished(otTcpEndpoint *aEndpoint) {
}
            
void fd_rpc_channel_thread_ot_TcpForwardProgress(otTcpEndpoint *aEndpoint, size_t aInSendBuffer, size_t aBacklog) {
}

void fd_rpc_channel_thread_ot_TcpSendDone(otTcpEndpoint *aEndpoint, otLinkedBuffer *aData) {   
    fd_rpc_channel_thread_send_done();
}

void fd_rpc_channel_thread_ot_TcpReceiveAvailable(otTcpEndpoint *aEndpoint, size_t aBytesAvailable, bool aEndOfStream, size_t aBytesRemaining) {           
    const otLinkedBuffer *data = NULL;
    otTcpReceiveByReference(aEndpoint, &data);
    size_t totalReceived = 0;
    for (; data != NULL; data = data->mNext) {
        totalReceived += data->mLength;
        fd_rpc_channel_thread_rx(data->mData, data->mLength);
    }
    otTcpCommitReceive(aEndpoint, totalReceived, 0);

    if (aEndOfStream) {
        otError error = otTcpAbort(&fd_rpc_channel_thread.ot.tcp.endpoint);
        fd_assert(error == OT_ERROR_NONE);
    }
}

void fd_rpc_channel_thread_ot_TcpDisconnected(otTcpEndpoint *aEndpoint, otTcpDisconnectedReason aReason) {
    otError error = otTcpAbort(&fd_rpc_channel_thread.ot.tcp.endpoint);
    fd_assert(error == OT_ERROR_NONE);

    fd_rpc_channel_thread_disconnected();
}

void fd_rpc_channel_thread_tcp_up(void) {
    otTcpEndpointInitializeArgs endpoint_initialize_args = {
        .mDisconnectedCallback = fd_rpc_channel_thread_ot_TcpDisconnected,
        .mEstablishedCallback = fd_rpc_channel_thread_ot_TcpEstablished,
        .mForwardProgressCallback = fd_rpc_channel_thread_ot_TcpForwardProgress,
        .mReceiveAvailableCallback = fd_rpc_channel_thread_ot_TcpReceiveAvailable,
        .mSendDoneCallback= fd_rpc_channel_thread_ot_TcpSendDone,
        .mReceiveBuffer = fd_rpc_channel_thread.ot.receive_buffer,
        .mReceiveBufferSize = sizeof(fd_rpc_channel_thread.ot.receive_buffer),
    };
    otError error = otTcpEndpointInitialize(fd_rpc_channel_thread.ot.instance, &fd_rpc_channel_thread.ot.tcp.endpoint, &endpoint_initialize_args);
    fd_assert(error == OT_ERROR_NONE);

    otTcpListenerInitializeArgs listener_initialize_args = {
        .mAcceptReadyCallback = fd_rpc_channel_thread_ot_TcpAcceptReady,
        .mAcceptDoneCallback= fd_rpc_channel_thread_ot_TcpAcceptDone,
    };
    error = otTcpListenerInitialize(fd_rpc_channel_thread.ot.instance, &fd_rpc_channel_thread.ot.tcp.listener, &listener_initialize_args);
    const otIp6Address *eid = otThreadGetMeshLocalEid(fd_rpc_channel_thread.ot.instance);
    fd_assert(eid != NULL);
    otSockAddr sock_addr = {
        .mAddress = *eid,
        .mPort = fd_rpc_channel_thread.configuration.service_port,
    };
    error = otTcpListen(&fd_rpc_channel_thread.ot.tcp.listener, &sock_addr);
    fd_assert(error == OT_ERROR_NONE);
}

void fd_rpc_channel_thread_udp_receive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo) {
    fd_rpc_channel_thread.ot.udp.message_info = *aMessageInfo; // !!! should probably do this only on connect -denis

    uint8_t data[1024 + 1];
    uint16_t offset = 0;
    uint16_t length = otMessageRead(aMessage, offset, data, sizeof(data));
    fd_assert(length <= 1024);
    fd_rpc_stream_receive(&fd_rpc_channel_thread.ot.udp.stream, data, length);
}

void fd_rpc_channel_thread_udp_up(void) {
    memset(&fd_rpc_channel_thread.ot.udp.socket, 0, sizeof(fd_rpc_channel_thread.ot.udp.socket));
    otUdpOpen(fd_rpc_channel_thread.ot.instance, &fd_rpc_channel_thread.ot.udp.socket, fd_rpc_channel_thread_udp_receive, NULL);
    otSockAddr sock_addr = {
        .mPort = fd_rpc_channel_thread.configuration.service_port,
    };
    otUdpBind(fd_rpc_channel_thread.ot.instance, &fd_rpc_channel_thread.ot.udp.socket, &sock_addr, OT_NETIF_THREAD);
}

void fd_rpc_channel_thread_up(void) {
    uint8_t id[8];
    ssize_t length = hwinfo_get_device_id(id, sizeof(id));
    fd_assert(length == sizeof(id));
    snprintf(
        fd_rpc_channel_thread.ot.host_name,
        sizeof(fd_rpc_channel_thread.ot.host_name),
        "%s-%02x%02x%02x%02x%02x%02x%02x%02x",
        fd_rpc_channel_thread.configuration.name,
        id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7]
    );
    otError error = otSrpClientSetHostName(fd_rpc_channel_thread.ot.instance, fd_rpc_channel_thread.ot.host_name);
    fd_assert((error == OT_ERROR_NONE) || (error == OT_ERROR_INVALID_STATE));
    const otIp6Address *eid = otThreadGetMeshLocalEid(fd_rpc_channel_thread.ot.instance);
    fd_assert(eid != NULL);
    error = otSrpClientSetHostAddresses(fd_rpc_channel_thread.ot.instance, eid, 1);
    otSrpClientSetCallback(fd_rpc_channel_thread.ot.instance, fd_rpc_channel_thread_ot_SrpClientCallback, NULL);
    fd_assert(error == OT_ERROR_NONE);
    otSrpClientEnableAutoStartMode(fd_rpc_channel_thread.ot.instance, fd_rpc_channel_thread_ot_SrpClientAutoStartCallback, NULL);

    fd_rpc_channel_thread.ot.protocol.up();

    otSrpClientService *service = &fd_rpc_channel_thread.ot.service;
    snprintf(fd_rpc_channel_thread.ot.service_name, sizeof(fd_rpc_channel_thread.ot.service_name), "_%s._tcp", fd_rpc_channel_thread.configuration.name);
    service->mName = fd_rpc_channel_thread.ot.service_name;
    snprintf(fd_rpc_channel_thread.ot.service_instance_name, sizeof(fd_rpc_channel_thread.ot.service_instance_name), "%s", fd_rpc_channel_thread.configuration.instance_name);
    service->mInstanceName = fd_rpc_channel_thread.ot.service_instance_name;
    service->mPort = fd_rpc_channel_thread.configuration.service_port;
    service->mPriority = 1;
    service->mWeight = 1;
    service->mNumTxtEntries = 0;
    error = otSrpClientAddService(fd_rpc_channel_thread.ot.instance, &fd_rpc_channel_thread.ot.service);
    fd_assert((error == OT_ERROR_NONE) || (error == OT_ERROR_ALREADY));

    fd_rpc_channel_thread_set_state(fd_rpc_channel_thread_state_listening);
}

void fd_rpc_channel_thread_tcp_down(void) {
    otError error = otTcpListenerDeinitialize(&fd_rpc_channel_thread.ot.tcp.listener);
    fd_assert(error == OT_ERROR_NONE);

    error = otTcpEndpointDeinitialize(&fd_rpc_channel_thread.ot.tcp.endpoint);
    fd_assert(error == OT_ERROR_NONE);
}

void fd_rpc_channel_thread_udp_down(void) {
    k_work_cancel_delayable(&fd_rpc_channel_thread.ot.udp.send_done_work);

    otError error = otUdpClose(fd_rpc_channel_thread.ot.instance, &fd_rpc_channel_thread.ot.udp.socket);
    fd_assert(error == OT_ERROR_NONE);
}

void fd_rpc_channel_thread_down(void) {
    otSrpClientDisableAutoStartMode(fd_rpc_channel_thread.ot.instance);

    if (fd_rpc_channel_thread.state > fd_rpc_channel_thread_state_started) {
        // Removing a service is an asynchronous sequence of events.
        // The subsequent otSrpClientStop does not allow time for this to complete.
        // On the next start it will complete, but interfere with adding the service.
        // So we just leave the service registered to wait for a TTL timeout.
        // -denis
        /*
        otError error = otSrpClientRemoveService(fd_rpc_channel_thread.ot.instance, &fd_rpc_channel_thread.ot.service);
        fd_assert(error == OT_ERROR_NONE);
        */

        otSrpClientStop(fd_rpc_channel_thread.ot.instance);

        fd_rpc_channel_thread.ot.protocol.down();
    }

    otError error = otThreadSetEnabled(fd_rpc_channel_thread.ot.instance, false);
    fd_assert(error == OT_ERROR_NONE);
    error = otIp6SetEnabled(fd_rpc_channel_thread.ot.instance, false);
    fd_assert(error == OT_ERROR_NONE);

    fd_rpc_channel_thread_set_state(fd_rpc_channel_thread_state_none);
}

void fd_rpc_channel_thread_stop(void) {
    if (fd_rpc_channel_thread.state != fd_rpc_channel_thread_state_none) {
        fd_rpc_channel_thread_down();
    }
}

void fd_rpc_channel_thread_set_dataset(otOperationalDataset dataset) {
    if (fd_rpc_channel_thread.state != fd_rpc_channel_thread_state_none) {
        fd_rpc_channel_thread_down();
    }

    otInstance *ot_instance = fd_rpc_channel_thread.ot.instance;

    /* Set the router selection jitter to override the 2 minute default.
       CLI cmd > routerselectionjitter 20
       Warning: For demo purposes only - not to be used in a real product */
    uint8_t jitterValue = 20;
    otThreadSetRouterSelectionJitter(ot_instance, jitterValue);
    
    otError error = otDatasetSetActive(ot_instance, &dataset);
    fd_assert(error == OT_ERROR_NONE);

    error = otIp6SetEnabled(ot_instance, true);
    fd_assert(error == OT_ERROR_NONE);
    error = otThreadSetEnabled(ot_instance, true);
    fd_assert(error == OT_ERROR_NONE);

    fd_rpc_channel_thread_set_state(fd_rpc_channel_thread_state_started);
}

bool fd_rpc_channel_thread_is_role_active(otDeviceRole role) {
    switch (role) {
        case OT_DEVICE_ROLE_LEADER:
        case OT_DEVICE_ROLE_ROUTER:
        case OT_DEVICE_ROLE_CHILD:
            return true;
        default:
            break;
    }
    return false;
}

void fd_rpc_channel_thread_ot_StateChanged(uint32_t flags, void *context) {
    if ((flags & OT_CHANGED_THREAD_ROLE) == 0) {
        return;
    }

    bool was_active = fd_rpc_channel_thread_is_role_active(fd_rpc_channel_thread.ot.role);
    fd_rpc_channel_thread.ot.role = otThreadGetDeviceRole(context);
    bool is_active = fd_rpc_channel_thread_is_role_active(fd_rpc_channel_thread.ot.role);
    if (!was_active && is_active) {
        fd_rpc_channel_thread_up();
    }
    if (was_active && !is_active) {
        fd_rpc_channel_thread_down();
    }
}

void fd_rpc_channel_thread_initialize_ot(void) {
    fd_rpc_channel_thread.ot.instance = otInstanceInitSingle();
    otSetStateChangedCallback(fd_rpc_channel_thread.ot.instance, fd_rpc_channel_thread_ot_StateChanged, fd_rpc_channel_thread.ot.instance);
}

void fd_rpc_channel_thread_udp_stream_received_connect(struct fd_rpc_stream_s *stream) {
	fd_rpc_channel_thread_connected();
}

void fd_rpc_channel_thread_udp_stream_received_disconnect(struct fd_rpc_stream_s *stream) {
	fd_rpc_channel_thread_disconnected();
}

void fd_rpc_channel_thread_udp_stream_received_keep_alive(struct fd_rpc_stream_s *stream) {
}

void fd_rpc_channel_thread_udp_stream_received_data(struct fd_rpc_stream_s *stream, const uint8_t *data, size_t length) {
    fd_rpc_channel_thread_rx(data, length);
}

void fd_rpc_channel_thread_udp_stream_received_ack(struct fd_rpc_stream_s *stream) {
    fd_rpc_channel_thread_send_done();
}

void fd_rpc_channel_thread_udp_stream_sent_disconnect(struct fd_rpc_stream_s *stream) {
	fd_rpc_channel_thread_disconnected();
}

bool fd_rpc_channel_thread_udp_stream_send(struct fd_rpc_stream_s *stream, const uint8_t *header, size_t header_length, const uint8_t *data, size_t length) {
    otMessage *message = otUdpNewMessage(fd_rpc_channel_thread.ot.instance, NULL);
    fd_assert(message != NULL);
    if (message == NULL) {
        return false;
    }

    otError error = OT_ERROR_NONE;
    if (header_length > 0) {
        error = otMessageAppend(message, header, header_length);
        fd_assert(error == OT_ERROR_NONE);
        if (error != OT_ERROR_NONE) {
            goto exit;
        }
    }
    if (length > 0) {
        error = otMessageAppend(message, data, length);
        fd_assert(error == OT_ERROR_NONE);
        if (error != OT_ERROR_NONE) {
            goto exit;
        }
    }
    error = otUdpSend(fd_rpc_channel_thread.ot.instance, &fd_rpc_channel_thread.ot.udp.socket, message, &fd_rpc_channel_thread.ot.udp.message_info);
    fd_assert(error == OT_ERROR_NONE);
    if (error != OT_ERROR_NONE) {
        goto exit;
    }

exit:
    if (error != OT_ERROR_NONE) {
        otMessageFree(message);
        return false;
    }
    return true;
}

bool fd_rpc_channel_thread_udp_stream_send_data(struct fd_rpc_stream_s *stream, const uint8_t *header, size_t header_length, const uint8_t *data, size_t length) {
    return fd_rpc_channel_thread_udp_stream_send(stream, header, header_length, data, length);
}

bool fd_rpc_channel_thread_udp_stream_send_command(struct fd_rpc_stream_s *stream, const uint8_t *data, size_t length) {
    return fd_rpc_channel_thread_udp_stream_send(stream, data, length, NULL, 0);
}

void fd_rpc_channel_thread_udp_stream_initialize(void) {
    static const fd_rpc_stream_client_t client = {
        fd_rpc_channel_thread_udp_stream_received_connect,
        fd_rpc_channel_thread_udp_stream_received_disconnect,
        fd_rpc_channel_thread_udp_stream_received_keep_alive,
        fd_rpc_channel_thread_udp_stream_received_data,
        fd_rpc_channel_thread_udp_stream_received_ack,
        fd_rpc_channel_thread_udp_stream_sent_disconnect,
        fd_rpc_channel_thread_udp_stream_send_command,
        fd_rpc_channel_thread_udp_stream_send_data,
    };
    fd_rpc_stream_initialize(&fd_rpc_channel_thread.ot.udp.stream, &client);
}

void fd_rpc_channel_thread_initialize(const fd_rpc_channel_thread_configuration_t *configuration) {
    fd_rpc_channel_thread.configuration = *configuration;

    switch (configuration->transport) {
        case fd_rpc_channel_thread_transport_tcp:
            fd_rpc_channel_thread.ot.protocol.up = fd_rpc_channel_thread_tcp_up;
            fd_rpc_channel_thread.ot.protocol.down = fd_rpc_channel_thread_tcp_down;
            fd_rpc_channel_thread.ot.protocol.send = fd_rpc_channel_thread_tcp_send;
            break;
        case fd_rpc_channel_thread_transport_udp:
            fd_rpc_channel_thread.ot.protocol.up = fd_rpc_channel_thread_udp_up;
            fd_rpc_channel_thread.ot.protocol.down = fd_rpc_channel_thread_udp_down;
            fd_rpc_channel_thread.ot.protocol.send = fd_rpc_channel_thread_udp_send;
            fd_rpc_channel_thread_udp_stream_initialize();
            break;
        default:
            fd_assert_fail("unsupported transport");
    }

    fd_rpc_channel_thread.ot.udp.send_delay = K_MSEC(10);
    k_work_init_delayable(&fd_rpc_channel_thread.ot.udp.send_done_work, fd_rpc_channel_thread_udp_send_done_work);

    fd_rpc_channel_thread.channel = (fd_rpc_channel_t) { 
        .packet_write = fd_rpc_channel_thread_packet_write,
        .get_free_space = fd_rpc_channel_thread_get_free_space,
        .get_rx_free_space = fd_rpc_channel_thread_get_rx_free_space,
        .set_free_space_increased_callback = fd_rpc_channel_thread_set_free_space_increased_callback,
    };

    fd_binary_initialize(&fd_rpc_channel_thread.rx_packet, fd_rpc_channel_thread.rx_packet_buffer, sizeof(fd_rpc_channel_thread.rx_packet_buffer));
    
    k_timer_init(&fd_rpc_channel_thread.timer, fd_rpc_channel_thread_timer, 0);

    k_work_init(&fd_rpc_channel_thread.rx_work, fd_rpc_channel_thread_rx_work);
    k_work_init(&fd_rpc_channel_thread.tx_work, fd_rpc_channel_thread_tx_work);

    k_work_init(&fd_rpc_channel_thread.connected_work, fd_rpc_channel_thread_connected_work);
    k_work_init(&fd_rpc_channel_thread.disconnected_work, fd_rpc_channel_thread_disconnected_work);

	fd_rpc_channel_thread_initialize_ot();
}

fd_source_pop()
