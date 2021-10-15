//
//  fd_rtc.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/14/21.
//

import Foundation

// int64_t fd_rtc_get_utc(void);
@_cdecl("fd_rtc_get_utc")
func fd_rtc_get_utc() -> Int64 {
    return Int64(Date().timeIntervalSince1970)
}
