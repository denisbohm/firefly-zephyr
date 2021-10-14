//
//  fd_timing.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/14/21.
//

import Foundation

// uint32_t fd_timing_get_timestamp(void);
@_cdecl("fd_timing_get_timestamp")
func fd_timing_get_timestamp() -> UInt32 {
    return 0
}
