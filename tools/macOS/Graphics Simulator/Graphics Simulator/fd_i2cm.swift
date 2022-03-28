//
//  fd_i2cm_macos.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/12/21.
//

import Foundation

class SimulateI2CMDevice {
    
    let address: UInt32
    
    init(address: UInt32) {
        self.address = address
    }
    
    func fd_i2cm_device_io(device: UnsafePointer<fd_i2cm_device_t>, io: UnsafePointer<fd_i2cm_io_t>) -> Bool {
        return false
    }
    
}

class SimulateI2CM {
    
    var devices: [UInt32: SimulateI2CMDevice]
    
    init() {
        self.devices = [:]
    }
    
    func addDevice(device: SimulateI2CMDevice) {
        self.devices[device.address] = device
    }
    
    func fd_i2cm_device_io(device: UnsafePointer<fd_i2cm_device_t>, io: UnsafePointer<fd_i2cm_io_t>) -> Bool {
        let address = device.pointee.address
        if !self.devices.keys.contains(address) {
            return false
        }
        let simulation = self.devices[address]!
        return simulation.fd_i2cm_device_io(device: device, io: io)
    }
    
}

var simulateI2CM = SimulateI2CM()

// void fd_i2cm_initialize(
//     const fd_i2cm_bus_t *buses, uint32_t bus_count,
//     const fd_i2cm_device_t *devices, uint32_t device_count
// );
@_cdecl("fd_i2cm_initialize")
func fd_i2cm_initialize(buses: UnsafePointer<fd_i2cm_bus_t>, bus_count: UInt32, devices: UnsafePointer<fd_i2cm_device_t>, device_count: UInt32) {
}

// const fd_i2cm_bus_t *fd_i2cm_get_bus(int index);
@_cdecl("fd_i2cm_get_bus")
func fd_i2cm_get_bus(index: Int) -> UnsafePointer<fd_i2cm_bus_t>? {
    return nil;
}

// const fd_i2cm_device_t *fd_i2cm_get_device(int index);
@_cdecl("fd_i2cm_get_device")
func fd_i2cm_get_device(index: Int) -> UnsafePointer<fd_i2cm_device_t>? {
    return nil;
}

// void fd_i2cm_clear_bus(const fd_i2cm_bus_t *bus);
@_cdecl("fd_i2cm_clear_bus")
func fd_i2cm_clear_bus(bus: fd_i2cm_bus_t) {
}

// void fd_i2cm_bus_enable(const fd_i2cm_bus_t *bus);
@_cdecl("fd_i2cm_bus_enable")
func fd_i2cm_bus_enable(bus: fd_i2cm_bus_t) {
}

// void fd_i2cm_bus_disable(const fd_i2cm_bus_t *bus);
@_cdecl("fd_i2cm_bus_disable")
func fd_i2cm_bus_disable(bus: fd_i2cm_bus_t) {
}

// bool fd_i2cm_bus_is_enabled(const fd_i2cm_bus_t *bus);
@_cdecl("fd_i2cm_bus_is_enabled")
func fd_i2cm_bus_is_enabled(bus: fd_i2cm_bus_t) -> Bool {
    return true
}

// start asynchronous I/O
// bool fd_i2cm_device_io(const fd_i2cm_device_t *device, const fd_i2cm_io_t *io);
@_cdecl("fd_i2cm_device_io")
func fd_i2cm_device_io(device: UnsafePointer<fd_i2cm_device_t>, io: UnsafePointer<fd_i2cm_io_t>) -> Bool {
    return simulateI2CM.fd_i2cm_device_io(device: device, io: io);
}

// wait for asynchronous I/O to complete
// bool fd_i2cm_bus_wait(const fd_i2cm_bus_t *bus);
@_cdecl("fd_i2cm_bus_wait")
func fd_i2cm_bus_wait(bus: fd_i2cm_bus_t) -> Bool {
    return true
}
