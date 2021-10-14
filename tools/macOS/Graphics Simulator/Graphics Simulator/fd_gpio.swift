//
//  fd_gpio_macos.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/12/21.
//

import Foundation

class SimulateGPIO {
    
    class Port {
        
        var input: UInt32 = 0
        var output: UInt32 = 0
        var callbacks = [fd_gpio_callback_t?](repeating: nil, count: 32)
        
        init() {
        }
        
    }
    
    var ports: [Port]
    
    init() {
        self.ports = [Port(), Port()]
    }
    
    func set_input(gpio: fd_gpio_t, value: Bool) {
        let port = self.ports[Int(gpio.port)]
        let current = (port.input & (1 << gpio.pin)) != 0
        if value == current {
            return
        }
        if value {
            port.input |= 1 << gpio.pin
        } else {
            port.input &= ~(1 << gpio.pin)
        }
        if let callback = port.callbacks[Int(gpio.pin)] {
            callback()
        }
    }

    func get_output(gpio: fd_gpio_t) -> Bool {
        let port = self.ports[Int(gpio.port)]
        return (port.output & (1 << gpio.pin)) != 0
    }
    
    func fd_gpio_set_callback(gpio: fd_gpio_t, callback: @escaping fd_gpio_callback_t) {
        let port = self.ports[Int(gpio.port)]
        port.callbacks[Int(gpio.pin)] = callback
    }
    
    func fd_gpio_set(gpio: fd_gpio_t, value: Bool) {
        let port = self.ports[Int(gpio.port)]
        let current = (port.output & (1 << gpio.pin)) != 0
        if value == current {
            return
        }
        if value {
            port.output |= 1 << gpio.pin
        } else {
            port.output &= ~(1 << gpio.pin)
        }
    }
    
    func fd_gpio_get(gpio: fd_gpio_t) -> Bool {
        let port = self.ports[Int(gpio.port)]
        return (port.input & (1 << gpio.pin)) != 0
    }
    
}

var simulateGPIO = SimulateGPIO()

// void fd_gpio_initialize(void);
@_cdecl("fd_gpio_initialize")
func fd_gpio_initialize() {
}

// void fd_gpio_configure_output(fd_gpio_t gpio);
@_cdecl("fd_gpio_configure_output")
func fd_gpio_configure_output(gpio: fd_gpio_t) {
}

//void fd_gpio_configure_input(fd_gpio_t gpio);
@_cdecl("fd_gpio_configure_input")
func fd_gpio_configure_input(gpio: fd_gpio_t) {
}

// void fd_gpio_set_callback(fd_gpio_t gpio, fd_gpio_callback_t callback);
@_cdecl("fd_gpio_set_callback")
func fd_gpio_set_callback(gpio: fd_gpio_t, callback: @escaping fd_gpio_callback_t) {
    simulateGPIO.fd_gpio_set_callback(gpio: gpio, callback: callback)
}

// void fd_gpio_set(fd_gpio_t gpio, bool value);
@_cdecl("fd_gpio_set")
func fd_gpio_set(gpio: fd_gpio_t, value: Bool) {
    simulateGPIO.fd_gpio_set(gpio: gpio, value: value)
}

// bool fd_gpio_get(fd_gpio_t gpio);
@_cdecl("fd_gpio_get")
func fd_gpio_get(gpio: fd_gpio_t) -> Bool {
    return simulateGPIO.fd_gpio_get(gpio: gpio)
}

// void fd_gpio_port_set_bits(int port, uint32_t bits);
@_cdecl("fd_gpio_port_set_bits")
func fd_gpio_port_set_bits(port: Int, bits: UInt32) {
}

// void fd_gpio_port_clear_bits(int port, uint32_t bits);
@_cdecl("fd_gpio_port_clear_bits")
func fd_gpio_port_clear_bits(port: Int, bits: UInt32) {
}

// uint32_t fd_gpio_port_get(int port);
@_cdecl("fd_gpio_port_get")
func fd_gpio_port_get(gpio: fd_gpio_t) -> UInt32 {
    return 0
}
