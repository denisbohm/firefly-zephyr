//
//  fd_uart_macos.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/12/21.
//

import Foundation

class SimulateUART {
    
    init() {
    }
    
}

var simulateUART = SimulateUART()

// void fd_uart_initialize(void);
@_cdecl("fd_uart_initialize")
func fd_uart_initialize() {
}

// void fd_uart_instance_initialize(fd_uart_instance_t *instance);
@_cdecl("fd_uart_instance_initialize")
func fd_uart_instance_initialize(instance: fd_uart_instance_t) {
}

// size_t fd_uart_instance_tx(fd_uart_instance_t *instance, const uint8_t *data, size_t length);
@_cdecl("fd_uart_instance_tx")
func fd_uart_instance_tx(instance: fd_uart_instance_t, data: UnsafePointer<UInt8>, length: size_t) -> size_t {
    return 0
}

// void fd_uart_instance_tx_flush(fd_uart_instance_t *instance);
@_cdecl("fd_uart_instance_tx_flush")
func fd_uart_instance_tx_flush(instance: fd_uart_instance_t) {
}

// size_t fd_uart_instance_rx(fd_uart_instance_t *instance, uint8_t *data, size_t length);
@_cdecl("fd_uart_instance_rx")
func fd_uart_instance_rx(instance: fd_uart_instance_t, data: UnsafePointer<UInt8>, length: size_t) -> size_t {
    return 0
}
