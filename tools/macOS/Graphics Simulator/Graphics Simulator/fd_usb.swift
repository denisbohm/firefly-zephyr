//
//  fd_usb.swift
//  Itamar Loop Simulator
//
//  Created by Denis Bohm on 12/20/21.
//

import Foundation

// void fd_usb_initialize(void);
@_cdecl("fd_usb_initialize")
func fd_usb_initialize() {
}

// void fd_usb_cdc_initialize(fd_usb_cdc_configuration_t configuration);
@_cdecl("fd_usb_cdc_initialize")
func fd_usb_cdc_initialize(configuration: fd_usb_cdc_configuration_t) {
}

// bool fd_usb_cdc_tx_data(const uint8_t *data, size_t length);
@_cdecl("fd_usb_cdc_tx_data")
func fd_usb_cdc_tx_data(data: UnsafePointer<UInt8>, length: size_t) -> Bool {
    return false
}

// void fd_usb_msc_initialize(void);
@_cdecl("fd_usb_msc_initialize")
func fd_usb_msc_initialize() {
}
