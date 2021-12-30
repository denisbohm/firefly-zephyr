//
//  fd_ssd1327_bus_macos.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/12/21.
//

import Foundation
import SwiftUI

class SimulateSSD1327 {
    
    static let CODE_SET_COLUMN_ADDRESS = UInt8(0x15)
    static let CODE_SET_ROW_ADDRESS    = UInt8(0x75)
    static let CODE_SET_DISPLAY_OFF    = UInt8(0xAE)
    static let CODE_SET_DISPLAY_ON     = UInt8(0xAF)

    class Command {
        
        let code: UInt8
        let length: Int
        
        init(code: UInt8, length: Int = 0) {
            self.code = code
            self.length = length
        }
        
        func decode(bytes: [UInt8]) {
        }
        
    }
    
    class CommandSetDisplayOn: Command {
        
        init() {
            super.init(code: CODE_SET_DISPLAY_ON)
        }
        
        override func decode(bytes: [UInt8]) {
            simulateSSD1327.set_display_on()
        }
        
    }
    
    class CommandSetDisplayOff: Command {
        
        init() {
            super.init(code: CODE_SET_DISPLAY_OFF)
        }
        
        override func decode(bytes: [UInt8]) {
            simulateSSD1327.set_display_off()
        }
        
    }
    
    class CommandSetAddress: Command {

        var start: Int = 0
        var end: Int = 127
        var current: Int = 0
        
        init(code: UInt8) {
            super.init(code: code, length: 2)
        }
        
        override func decode(bytes: [UInt8]) {
            start = Int(bytes[0])
            end = Int(bytes[1])
            current = start
        }
        
        func advance() -> Bool {
            let rollover = current >= end
            if rollover {
                current = start
            } else {
                current += 1
            }
            return rollover
        }

    }
    
    var dcxGpio = fd_gpio_t(port: 0, pin: 26)
    var commandsByCode: [UInt8:Command] = [:]
    var command: Command? = nil
    var bytes: [UInt8] = []
    let columnAddress = CommandSetAddress(code: SimulateSSD1327.CODE_SET_COLUMN_ADDRESS)
    let rowAddress = CommandSetAddress(code: SimulateSSD1327.CODE_SET_ROW_ADDRESS)
    var imageRep: NSBitmapImageRep!
    var image: NSImage!
    var modified = true
    
    init() {
        let commands: [Command] = [
            columnAddress,
            rowAddress,
            CommandSetDisplayOn(),
            CommandSetDisplayOff(),
            Command(code: 0x81, length: 1),
            Command(code: 0xa0, length: 1),
            Command(code: 0xa1, length: 1),
            Command(code: 0xa2, length: 1),
            Command(code: 0xa4),
            Command(code: 0xa8, length: 1),
            Command(code: 0xab, length: 1),
            Command(code: 0xb1, length: 1),
            Command(code: 0xb3, length: 1),
            Command(code: 0xb6, length: 1),
            Command(code: 0xbc, length: 1),
            Command(code: 0xbe, length: 1),
            Command(code: 0xd5, length: 1),
            Command(code: 0xfd, length: 1),
        ]
        for command in commands {
            commandsByCode[command.code] = command
        }
        
        imageRep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: 128, pixelsHigh: 96, bitsPerSample: 8, samplesPerPixel: 4, hasAlpha: true, isPlanar: false, colorSpaceName: NSColorSpaceName.deviceRGB, bytesPerRow: 0, bitsPerPixel: 0)
        self.clear()
        
        image = NSImage(size: imageRep.size)
        image.addRepresentation(imageRep)
    }
    
    func clear() {
        var bitmapData = imageRep.bitmapData!
        for _ in 0 ..< 128 * 96 {
            bitmapData.pointee = UInt8(0x00)
            bitmapData += 1
            bitmapData.pointee = UInt8(0x00)
            bitmapData += 1
            bitmapData.pointee = UInt8(0x00)
            bitmapData += 1
            bitmapData.pointee = UInt8(0x00)
            bitmapData += 1
        }
    }
    
    func set_display_on() {
        modified = true
    }

    func set_display_off() {
        self.clear()

        modified = true
    }

    func writePixel(x: Int, y: Int, r: UInt8, g: UInt8, b: UInt8) {
        let px = 128 - x - 1
        let py = 96 - y - 1
        var bitmapData = imageRep.bitmapData! + (py * 128 + px) * 4
        bitmapData.pointee = r
        bitmapData += 1
        bitmapData.pointee = g
        bitmapData += 1
        bitmapData.pointee = b
        bitmapData += 1
        let a = UInt8(0xff)
        bitmapData.pointee = a
        bitmapData += 1

        modified = true
    }

    func processCommand(byte: UInt8) {
        if command == nil {
            command = commandsByCode[byte]
            bytes = []
        } else {
            bytes.append(byte)
        }
        if bytes.count < (command?.length ?? 0) {
            return
        }
        
        command?.decode(bytes: bytes)
        
        command = nil
        bytes = []
    }
    
    func processData(byte: UInt8) {
        let p0 = byte & 0xf0
        let p1 = (byte & 0x0f) << 4
        let x = 2 * columnAddress.current
        let y = rowAddress.current
        writePixel(x: x, y: y, r: p0, g: p0, b: p0)
        writePixel(x: x + 1, y: y, r: p1, g: p1, b: p1)

        if columnAddress.advance() {
            let _ = rowAddress.advance()
        }
    }
    
    func write(byte: UInt8) {
        let dcx = simulateGPIO.get_output(gpio: self.dcxGpio)
        if dcx {
            processData(byte: byte)
        } else {
            processCommand(byte: byte)
        }
    }
    
    func fd_ssd1327_bus_write(data: UnsafePointer<UInt8>, length: UInt32) {
        for i in 0 ..< Int(length) {
            self.write(byte: data[i])
        }
    }

}

let simulateSSD1327 = SimulateSSD1327()

// void fd_ssd1327_bus_initialize(void);
@_cdecl("fd_ssd1327_bus_initialize")
func fd_ssd1327_bus_initialize() {
}

// void fd_ssd1327_bus_write(const uint8_t *data, uint32_t length);
@_cdecl("fd_ssd1327_bus_write")
func fd_ssd1327_bus_write(data: UnsafePointer<UInt8>, length: UInt32) {
    simulateSSD1327.fd_ssd1327_bus_write(data: data, length: length)
}
