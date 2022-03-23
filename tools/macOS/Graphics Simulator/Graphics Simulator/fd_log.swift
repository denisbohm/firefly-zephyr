//
//  fd_log_macos.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/12/21.
//

import Foundation

class SimulateLog {
    
    init() {
    }
    
}

var simulateLog = SimulateLog()

// void fd_log_initialize(void);
@_cdecl("fd_log_initialize")
func fd_log_initialize() {
}

// size_t fd_log_get(uint8_t *data, size_t size);
@_cdecl("fd_log_get")
func fd_log_get(data: UnsafePointer<UInt8>, size: size_t) -> size_t {
    return 0
}
