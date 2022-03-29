//
//  fd_uart_macos.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/12/21.
//

import Foundation

class SimulateUARTDevice {
    
    let name: String
    
    init(name: String) {
        self.name = name
    }
    
    func fd_uart_instance_tx(data: Data) -> size_t {
        return 0
    }

    func fd_uart_instance_tx_flush() {
    }

    func fd_uart_instance_rx(length: size_t) -> Data {
        return Data()
    }

}

class SimulateUART {
    
    var devices: [String: SimulateUARTDevice]
    
    init() {
        self.devices = [:]
    }
    
    func addDevice(device: SimulateUARTDevice) {
        self.devices[device.name] = device
    }
    
    func fd_uart_instance_tx(instance: UnsafePointer<fd_uart_instance_t>, data: UnsafePointer<UInt8>, length: size_t) -> size_t {
        let name = String(cString: instance.pointee.uart_device_name)
        if !self.devices.keys.contains(name) {
            return 0
        }
        let simulation = self.devices[name]!
        let simulationData = Data(bytes: data, count: length)
        return simulation.fd_uart_instance_tx(data: simulationData)
    }

    func fd_uart_instance_tx_flush(instance: UnsafePointer<fd_uart_instance_t>) {
        let name = String(cString: instance.pointee.uart_device_name)
        if !self.devices.keys.contains(name) {
            return
        }
        let simulation = self.devices[name]!
        simulation.fd_uart_instance_tx_flush()
    }

    func fd_uart_instance_rx(instance: UnsafePointer<fd_uart_instance_t>, data: UnsafeMutablePointer<UInt8>, length: size_t) -> size_t {
        let name = String(cString: instance.pointee.uart_device_name)
        if !self.devices.keys.contains(name) {
            return 0
        }
        let simulation = self.devices[name]!
        let simulationData = simulation.fd_uart_instance_rx(length: length)
        simulationData.copyBytes(to: data, count: simulationData.count)
        return simulationData.count
    }

}

var simulateUART = SimulateUART()

// void fd_uart_initialize(void);
@_cdecl("fd_uart_initialize")
func fd_uart_initialize() {
}

// void fd_uart_instance_initialize(fd_uart_instance_t *instance);
@_cdecl("fd_uart_instance_initialize")
func fd_uart_instance_initialize(instance: UnsafePointer<fd_uart_instance_t>) {
}

// void fd_uart_instance_configure(fd_uart_instance_t *instance, const fd_uart_configuration_t *configuration);
@_cdecl("fd_uart_instance_configure")
func fd_uart_instance_configure(instance: UnsafePointer<fd_uart_instance_t>, configuration: UnsafePointer<fd_uart_configuration_t>) {
}

// size_t fd_uart_instance_tx(fd_uart_instance_t *instance, const uint8_t *data, size_t length);
@_cdecl("fd_uart_instance_tx")
func fd_uart_instance_tx(instance: UnsafePointer<fd_uart_instance_t>, data: UnsafePointer<UInt8>, length: size_t) -> size_t {
    return simulateUART.fd_uart_instance_tx(instance: instance, data: data, length: length)
}

// void fd_uart_instance_tx_flush(fd_uart_instance_t *instance);
@_cdecl("fd_uart_instance_tx_flush")
func fd_uart_instance_tx_flush(instance: UnsafePointer<fd_uart_instance_t>) {
    simulateUART.fd_uart_instance_tx_flush(instance: instance)
}

// size_t fd_uart_instance_rx(fd_uart_instance_t *instance, uint8_t *data, size_t length);
@_cdecl("fd_uart_instance_rx")
func fd_uart_instance_rx(instance: UnsafePointer<fd_uart_instance_t>, data: UnsafeMutablePointer<UInt8>, length: size_t) -> size_t {
    return simulateUART.fd_uart_instance_rx(instance: instance, data: data, length: length)
}
