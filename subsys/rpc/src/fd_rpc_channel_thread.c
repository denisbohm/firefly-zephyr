#include "fd_rpc_channel_thread.h"

#include "fd_assert.h"
#include "fd_binary.h"
#include "fd_rpc.h"
#include "fd_unused.h"
#include "fd_usb.h"

#include <openthread/dns_client.h>
#include <openthread/instance.h>
#include <openthread/message.h>
#include <openthread/srp_client.h>
#include <openthread/tcp.h>
#include <openthread/thread.h>
#include <openthread/thread_ftd.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/ring_buffer.h>

#include <stdio.h>
#include <string.h>

fd_source_push()

typedef struct {
    otInstance *instance;
    otDeviceRole role;

    otTcpEndpoint endpoint;
    uint8_t receive_buffer[1024];
    uint8_t send_buffer[CONFIG_FIREFLY_SUBSYS_RPC_USB_TX_BUFFER_SIZE];
    otLinkedBuffer send_linked_buffer;
    volatile bool is_send_pending;

    otTcpListener listener;

    char host_name[128];
    char service_name[128];
    char service_instance_name[128];
    otSrpClientService service;
} fd_rpc_channel_thread_ot_t;

typedef struct {
    fd_rpc_channel_thread_configuration_t configuration;
    fd_rpc_channel_thread_consumer_t consumer;

    fd_rpc_channel_thread_state_t state;

    fd_rpc_channel_t channel;
    fd_rpc_channel_free_space_increased_callback_t free_space_increased_callback;
    struct k_work connected_work;
    struct k_work disconnected_work;
    struct k_work rx_work;
    struct k_work tx_work;

    uint8_t rx_ring_buffer[CONFIG_FIREFLY_SUBSYS_RPC_USB_RX_BUFFER_SIZE];
    struct ring_buf rx_ringbuf;
    uint8_t tx_ring_buffer[CONFIG_FIREFLY_SUBSYS_RPC_USB_TX_BUFFER_SIZE];
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

    if (fd_rpc_channel_thread.consumer.status_changed != NULL) {
        fd_rpc_channel_thread.consumer.status_changed(fd_rpc_channel_thread_get_status());
    }
}

void fd_rpc_channel_thread_rx_work(struct k_work *context fd_unused) {
    uint8_t data[64];
    while (true) {
        size_t length = ring_buf_get(&fd_rpc_channel_thread.rx_ringbuf, data, sizeof(data));
        if (length <= 0) {
            break;
        }
        fd_rpc_channel_received_data(&fd_rpc_channel_thread.channel, &fd_rpc_channel_thread.rx_packet, data, length);
    }
}

void fd_rpc_channel_thread_rx(const uint8_t *data, size_t length) {
    int rb_length = ring_buf_put(&fd_rpc_channel_thread.rx_ringbuf, data, length);
    fd_assert(rb_length == length);
    if (rb_length < length) {
        // Dropped recv_len - rb_len bytes
    }
    k_work_submit_to_queue(fd_rpc_channel_thread.configuration.work_queue, &fd_rpc_channel_thread.rx_work);
}

void fd_rpc_channel_thread_connected_work(struct k_work *work fd_unused) {
    fd_rpc_channel_opened(&fd_rpc_channel_thread.channel);

    fd_rpc_channel_thread_set_state(fd_rpc_channel_thread_state_connected);

    if (fd_rpc_channel_thread.consumer.connected != NULL) {
        fd_rpc_channel_thread.consumer.connected();
    }
}

void fd_rpc_channel_thread_disconnected_work(struct k_work *work fd_unused) {
    fd_rpc_channel_closed(&fd_rpc_channel_thread.channel);

    fd_rpc_channel_thread_set_state(fd_rpc_channel_thread_state_listening);

    if (fd_rpc_channel_thread.consumer.disconnected != NULL) {
        fd_rpc_channel_thread.consumer.disconnected();
    }
}

void fd_rpc_channel_thread_tx(void) {
    if (fd_rpc_channel_thread.ot.is_send_pending) {
        return;
    }

    uint8_t *buffer = NULL;
    uint32_t length = ring_buf_get_claim(&fd_rpc_channel_thread.tx_ringbuf, &buffer, sizeof(fd_rpc_channel_thread.tx_ring_buffer));
    if (length == 0) {
        int result = ring_buf_get_finish(&fd_rpc_channel_thread.tx_ringbuf, length);
        fd_assert(result == 0);

        if (fd_rpc_channel_thread.free_space_increased_callback != NULL) {
            fd_rpc_channel_thread.free_space_increased_callback(&fd_rpc_channel_thread.channel);
        }
        return;
    }

    fd_rpc_channel_thread.ot.is_send_pending = true;
    memcpy(fd_rpc_channel_thread.ot.send_buffer, buffer, length);
    fd_rpc_channel_thread.ot.send_linked_buffer.mData = fd_rpc_channel_thread.ot.send_buffer;
    fd_rpc_channel_thread.ot.send_linked_buffer.mLength = length;
    uint32_t flags = 0;
    otError error = otTcpSendByReference(&fd_rpc_channel_thread.ot.endpoint, &fd_rpc_channel_thread.ot.send_linked_buffer, flags);
    fd_assert(error == OT_ERROR_NONE);

    int result = ring_buf_get_finish(&fd_rpc_channel_thread.tx_ringbuf, length);
    fd_assert(result == 0);

    ++fd_rpc_channel_thread.tx_thread_count;
    fd_rpc_channel_thread.tx_thread_bytes += length;
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

    k_work_submit_to_queue(fd_rpc_channel_thread.configuration.work_queue, &fd_rpc_channel_thread.tx_work);
    return true;
}

size_t fd_rpc_channel_thread_get_free_space(void) {
    uint32_t space = ring_buf_space_get(&fd_rpc_channel_thread.tx_ringbuf);
    return space;
}

void fd_rpc_channel_thread_set_free_space_increased_callback(fd_rpc_channel_free_space_increased_callback_t callback) {
    fd_rpc_channel_thread.free_space_increased_callback = callback;
}

void fd_rpc_channel_thread_set_consumer(const fd_rpc_channel_thread_consumer_t *consumer) {
    fd_rpc_channel_thread.consumer = *consumer;
}

otTcpIncomingConnectionAction fd_rpc_channel_thread_ot_TcpAcceptReady(otTcpListener *aListener, const otSockAddr *aPeer, otTcpEndpoint **aAcceptInto) {
    *aAcceptInto = &fd_rpc_channel_thread.ot.endpoint;
    return OT_TCP_INCOMING_CONNECTION_ACTION_ACCEPT;
}

void fd_rpc_channel_thread_ot_TcpAcceptDone(otTcpListener *aListener, otTcpEndpoint *aEndpoint, const otSockAddr *aPeer) {
    fd_rpc_channel_thread.is_connected = true;
    k_work_submit_to_queue(fd_rpc_channel_thread.configuration.work_queue, &fd_rpc_channel_thread.connected_work);
}

void fd_rpc_channel_thread_ot_TcpEstablished(otTcpEndpoint *aEndpoint) {
}
            
void fd_rpc_channel_thread_ot_TcpForwardProgress(otTcpEndpoint *aEndpoint, size_t aInSendBuffer, size_t aBacklog) {
}

void fd_rpc_channel_thread_ot_TcpSendDone(otTcpEndpoint *aEndpoint, otLinkedBuffer *aData) {   
    fd_rpc_channel_thread.ot.is_send_pending = false;
    k_work_submit_to_queue(fd_rpc_channel_thread.configuration.work_queue, &fd_rpc_channel_thread.tx_work);
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
}
            
void fd_rpc_channel_thread_ot_TcpDisconnected(otTcpEndpoint *aEndpoint, otTcpDisconnectedReason aReason) {
    fd_rpc_channel_thread.is_connected = false;
    k_work_submit_to_queue(fd_rpc_channel_thread.configuration.work_queue, &fd_rpc_channel_thread.disconnected_work);
}       

void fd_rpc_channel_thread_up(void) {
    otTcpEndpointInitializeArgs endpoint_initialize_args = {
        .mDisconnectedCallback = fd_rpc_channel_thread_ot_TcpDisconnected,
        .mEstablishedCallback = fd_rpc_channel_thread_ot_TcpEstablished,
        .mForwardProgressCallback = fd_rpc_channel_thread_ot_TcpForwardProgress,
        .mReceiveAvailableCallback = fd_rpc_channel_thread_ot_TcpReceiveAvailable,
        .mSendDoneCallback= fd_rpc_channel_thread_ot_TcpSendDone,
        .mReceiveBuffer = fd_rpc_channel_thread.ot.receive_buffer,
        .mReceiveBufferSize = sizeof(fd_rpc_channel_thread.ot.receive_buffer),
    };
    otError error = otTcpEndpointInitialize(fd_rpc_channel_thread.ot.instance, &fd_rpc_channel_thread.ot.endpoint, &endpoint_initialize_args);
    fd_assert(error == OT_ERROR_NONE);

    otTcpListenerInitializeArgs listener_initialize_args = {
        .mAcceptReadyCallback = fd_rpc_channel_thread_ot_TcpAcceptReady,
        .mAcceptDoneCallback= fd_rpc_channel_thread_ot_TcpAcceptDone,
    };
    error = otTcpListenerInitialize(fd_rpc_channel_thread.ot.instance, &fd_rpc_channel_thread.ot.listener, &listener_initialize_args);
    const otIp6Address *eid = otThreadGetMeshLocalEid(fd_rpc_channel_thread.ot.instance);
    fd_assert(eid != NULL);
    otSockAddr sock_addr = {
        .mAddress = *eid,
        .mPort = fd_rpc_channel_thread.configuration.service_port,
    };
    error = otTcpListen(&fd_rpc_channel_thread.ot.listener, &sock_addr);
    fd_assert(error == OT_ERROR_NONE);

    otSrpClientService *service = &fd_rpc_channel_thread.ot.service;
    snprintf(fd_rpc_channel_thread.ot.service_name, sizeof(fd_rpc_channel_thread.ot.service_name), "_%s._tcp", fd_rpc_channel_thread.configuration.name);
    service->mName = fd_rpc_channel_thread.ot.service_name;
    snprintf(fd_rpc_channel_thread.ot.service_instance_name, sizeof(fd_rpc_channel_thread.ot.service_instance_name), "%s-service", fd_rpc_channel_thread.configuration.name);
    service->mInstanceName = fd_rpc_channel_thread.ot.service_instance_name;
    service->mPort = sock_addr.mPort;
    service->mPriority = 1;
    service->mWeight = 1;
    service->mNumTxtEntries = 0;
    error = otSrpClientAddService(fd_rpc_channel_thread.ot.instance, &fd_rpc_channel_thread.ot.service);
    fd_assert(error == OT_ERROR_NONE);

    fd_rpc_channel_thread_set_state(fd_rpc_channel_thread_state_listening);
}

void fd_rpc_channel_thread_down(void) {
}

void fd_rpc_channel_thread_ot_SrpClientCallback(
    otError aError,
    const otSrpClientHostInfo *aHostInfo,
    const otSrpClientService *aServices,
    const otSrpClientService *aRemovedServices,
    void *aContext
) {
    fd_assert(aError == OT_ERROR_NONE);
    for (const otSrpClientService *service = aServices; service != NULL; service = service->mNext) {
        if (service == &fd_rpc_channel_thread.ot.service) {
            static int count = 0;
            ++count;
            break;
        }
    }
}

void fd_rpc_channel_thread_ot_SrpClientAutoStartCallback(const otSockAddr *aServerSockAddr, void *aContext) {
    fd_assert(aServerSockAddr != NULL);
}

void fd_rpc_channel_thread_set_dataset(otOperationalDataset dataset) {
    otInstance *ot_instance = fd_rpc_channel_thread.ot.instance;

    otError error = otThreadSetEnabled(ot_instance, false);
    fd_assert(error == OT_ERROR_NONE);
    error = otIp6SetEnabled(ot_instance, false);
    fd_assert(error == OT_ERROR_NONE);

    /* Set the router selection jitter to override the 2 minute default.
       CLI cmd > routerselectionjitter 20
       Warning: For demo purposes only - not to be used in a real product */
    uint8_t jitterValue = 20;
    otThreadSetRouterSelectionJitter(ot_instance, jitterValue);
    
    error = otDatasetSetActive(ot_instance, &dataset);
    fd_assert(error == OT_ERROR_NONE);

    error = otIp6SetEnabled(ot_instance, true);
    fd_assert(error == OT_ERROR_NONE);
    error = otThreadSetEnabled(ot_instance, true);
    fd_assert(error == OT_ERROR_NONE);

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
    error = otSrpClientSetHostName(fd_rpc_channel_thread.ot.instance, fd_rpc_channel_thread.ot.host_name);
    fd_assert(error == OT_ERROR_NONE);
    const otIp6Address *eid = otThreadGetMeshLocalEid(fd_rpc_channel_thread.ot.instance);
    fd_assert(eid != NULL);
    error = otSrpClientSetHostAddresses(fd_rpc_channel_thread.ot.instance, eid, 1);
    otSrpClientSetCallback(fd_rpc_channel_thread.ot.instance, fd_rpc_channel_thread_ot_SrpClientCallback, NULL);
    fd_assert(error == OT_ERROR_NONE);
    otSrpClientEnableAutoStartMode(ot_instance, fd_rpc_channel_thread_ot_SrpClientAutoStartCallback, NULL);
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
        fd_rpc_channel_thread_set_state(fd_rpc_channel_thread_state_started);
    }
    if (was_active && !is_active) {
        fd_rpc_channel_thread_down();
        fd_rpc_channel_thread_set_state(fd_rpc_channel_thread_state_none);
    }
}

void fd_rpc_channel_thread_initialize_ot(void) {
    fd_rpc_channel_thread.ot.instance = otInstanceInitSingle();
    otSetStateChangedCallback(fd_rpc_channel_thread.ot.instance, fd_rpc_channel_thread_ot_StateChanged, fd_rpc_channel_thread.ot.instance);
}

void fd_rpc_channel_thread_initialize(const fd_rpc_channel_thread_configuration_t *configuration) {
    fd_rpc_channel_thread.configuration = *configuration;

    fd_rpc_channel_thread.channel = (fd_rpc_channel_t) { 
        .packet_write = fd_rpc_channel_thread_packet_write,
        .get_free_space = fd_rpc_channel_thread_get_free_space,
        .set_free_space_increased_callback = fd_rpc_channel_thread_set_free_space_increased_callback,
    };

    fd_binary_initialize(&fd_rpc_channel_thread.rx_packet, fd_rpc_channel_thread.rx_packet_buffer, sizeof(fd_rpc_channel_thread.rx_packet_buffer));
    
    k_work_init(&fd_rpc_channel_thread.rx_work, fd_rpc_channel_thread_rx_work);
    k_work_init(&fd_rpc_channel_thread.tx_work, fd_rpc_channel_thread_tx_work);

	ring_buf_init(&fd_rpc_channel_thread.rx_ringbuf, sizeof(fd_rpc_channel_thread.rx_ring_buffer), fd_rpc_channel_thread.rx_ring_buffer);
	ring_buf_init(&fd_rpc_channel_thread.tx_ringbuf, sizeof(fd_rpc_channel_thread.tx_ring_buffer), fd_rpc_channel_thread.tx_ring_buffer);

    k_work_init(&fd_rpc_channel_thread.connected_work, fd_rpc_channel_thread_connected_work);
    k_work_init(&fd_rpc_channel_thread.disconnected_work, fd_rpc_channel_thread_disconnected_work);

	fd_rpc_channel_thread_initialize_ot();
}

fd_source_pop()
