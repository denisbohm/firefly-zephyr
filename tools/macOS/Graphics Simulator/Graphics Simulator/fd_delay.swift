//
//  fd_delay_macos.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/12/21.
//

import Foundation

// void fd_delay_ms(uint32_t ms);
@_cdecl("fd_delay_ms")
func fd_delay_ms(ms: UInt32) {
}

// void fd_delay_us(uint32_t us);
@_cdecl("fd_delay_us")
func fd_delay_us(us: UInt32) {
}

// void fd_delay_ns(uint32_t ns);
@_cdecl("fd_delay_ns")
func fd_delay_ns(ns: UInt32) {
}
