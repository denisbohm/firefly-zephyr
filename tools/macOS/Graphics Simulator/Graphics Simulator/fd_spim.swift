//
//  fd_spim.swift
//  Itamar Loop Simulator
//
//  Created by Denis Bohm on 12/20/21.
//

import Foundation

// void fd_spim_initialize(
//     const fd_spim_bus_t *buses, uint32_t bus_count,
//     const fd_spim_device_t *devices, uint32_t device_count
// );
@_cdecl("fd_spim_initialize")
func fd_spim_initialize(buses: fd_spim_bus_t, bus_count: UInt32, devices: fd_spim_device_t, device_count: UInt32) {
}

// const fd_spim_bus_t *fd_spim_get_bus(int index);
@_cdecl("fd_spim_get_bus")
func fd_spim_get_bus(index: Int) -> UnsafePointer<fd_spim_bus_t>? {
    return nil;
}

// const fd_spim_device_t *fd_spim_get_device(int index);
@_cdecl("fd_spim_get_device")
func fd_spim_get_device(index: Int) -> UnsafePointer<fd_spim_device_t>? {
    return nil;
}

// void fd_spim_clear_bus(const fd_spim_bus_t *bus);
@_cdecl("fd_spim_clear_bus")
func fd_spim_clear_bus(bus: fd_spim_bus_t) {
}

// void fd_spim_bus_enable(const fd_spim_bus_t *bus);
@_cdecl("fd_spim_bus_enable")
func fd_spim_bus_enable(bus: fd_spim_bus_t) {
}

// void fd_spim_bus_disable(const fd_spim_bus_t *bus);
@_cdecl("fd_spim_bus_disable")
func fd_spim_bus_disable(bus: fd_spim_bus_t) {
}

// bool fd_spim_bus_is_enabled(const fd_spim_bus_t *bus);
@_cdecl("fd_spim_bus_is_enabled")
func fd_spim_bus_is_enabled(bus: fd_spim_bus_t) -> Bool {
    return true
}

// void fd_spim_device_select(const fd_spim_device_t *device);
@_cdecl("fd_spim_device_select")
func fd_spim_device_select(device: fd_spim_device_t) {
}

// void fd_spim_device_deselect(const fd_spim_device_t *device);
@_cdecl("fd_spim_device_deselect")
func fd_spim_device_deselect(device: fd_spim_device_t) {
}

// bool fd_spim_device_is_selected(const fd_spim_device_t *device);
@_cdecl("fd_spim_device_is_selected")
func fd_spim_device_is_selected(device: fd_spim_device_t) -> Bool {
    return true
}

// start asynchronous I/O
// bool fd_spim_bus_io(const fd_spim_bus_t *bus, const fd_spim_io_t *io);
@_cdecl("fd_spim_bus_io")
func fd_spim_bus_io(bus: fd_spim_bus_t, io: fd_spim_io_t) -> Bool {
    return true
}

// wait for asynchronous I/O to complete
// bool fd_spim_bus_wait(const fd_spim_bus_t *bus);
@_cdecl("fd_spim_bus_wait")
func fd_spim_bus_wait(bus: fd_spim_bus_t) -> Bool {
    return true
}
