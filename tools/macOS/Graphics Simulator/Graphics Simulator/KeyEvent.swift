//
//  KeyView.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/14/21.
//

import SwiftUI

class KeyEventView: NSView {
    
    override var acceptsFirstResponder: Bool { true }
    
    func key(event: NSEvent, value: Bool) {
        if event.isARepeat {
            return
        }
        let keyCode = event.keyCode
        switch keyCode {
        case 0x007b: // Left Arrow
            simulateGPIO.set_input(gpio: fd_gpio_t(port: 0, pin: 9), value: value)
        case 0x007c: // Right Arrow
            simulateGPIO.set_input(gpio: fd_gpio_t(port: 1, pin: 10), value: value)
        default:
            break
        }
    }
    
    override func keyDown(with event: NSEvent) {
        key(event: event, value: true)
    }
    
    override func keyUp(with event: NSEvent) {
        key(event: event, value: false)
    }
    
}

struct KeyEventHandling: NSViewRepresentable {
    
    func makeNSView(context: Context) -> NSView {
        let view = KeyEventView()
        DispatchQueue.main.async { // wait till next event cycle
            view.window?.makeFirstResponder(view)
        }
        return view
    }

    func updateNSView(_ nsView: NSView, context: Context) {
    }
    
}
