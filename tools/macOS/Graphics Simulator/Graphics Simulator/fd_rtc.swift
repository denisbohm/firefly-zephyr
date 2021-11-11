//
//  fd_rtc.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/14/21.
//

import Foundation

// fd_rtc_initialize(void);
@_cdecl("fd_rtc_initialize")
func fd_rtc_initialize() {
}

// int64_t fd_rtc_get_utc(void);
@_cdecl("fd_rtc_set_utc")
func fd_rtc_set_utc(utc: Int64) {
}

// bool fd_rtc_is_set(void);
@_cdecl("fd_rtc_is_set")
func fd_rtc_is_set() -> Bool {
    return true
}

// int64_t fd_rtc_get_utc(void);
@_cdecl("fd_rtc_get_utc")
func fd_rtc_get_utc() -> Int64 {
    return Int64(Date().timeIntervalSince1970)
}

// double fd_rtc_get_utc_precise(void);
@_cdecl("fd_rtc_get_utc_precise")
func fd_rtc_get_utc_precise() -> Double {
    return Double(Date().timeIntervalSince1970)
}
