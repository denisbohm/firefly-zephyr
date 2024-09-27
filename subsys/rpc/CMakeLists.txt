set(OVERLAY_CONFIG Kconfig)

zephyr_include_directories(include)
zephyr_include_directories(protobuf)

if(CONFIG_FIREFLY_SUBSYS_RPC)

zephyr_sources(src/fd_rpc.c)
zephyr_sources(src/fd_rpc_channel_usb.c)
zephyr_sources(src/fd_rpc_server_hardware.c)
zephyr_sources(src/fd_rpc_server_rtc.c)
zephyr_sources(src/fd_rpc_server_ux.c)

if(CONFIG_FIREFLY_SUBSYS_RPC_THREAD)
zephyr_sources(src/fd_rpc_channel_thread.c)
endif()

endif()

if(CONFIG_FIREFLY_SUBSYS_RPC_PROTOBUF)

zephyr_sources(protobuf/fd_rpc_service_hardware.pb.c)
zephyr_sources(protobuf/fd_rpc_service_rtc.pb.c)
zephyr_sources(protobuf/fd_rpc_service_ux.pb.c)

zephyr_sources(protobuf/hardware.pb.c)
zephyr_sources(protobuf/rtc.pb.c)
zephyr_sources(protobuf/ux.pb.c)

endif()
