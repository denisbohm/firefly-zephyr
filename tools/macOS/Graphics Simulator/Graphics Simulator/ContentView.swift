//
//  ContentView.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/12/21.
//

import SwiftUI

struct KeyEventHandling: NSViewRepresentable {
    
    class KeyView: NSView {
        
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

    func makeNSView(context: Context) -> NSView {
        let view = KeyView()
        DispatchQueue.main.async { // wait till next event cycle
            view.window?.makeFirstResponder(view)
        }
        return view
    }

    func updateNSView(_ nsView: NSView, context: Context) {
    }
    
}

struct ContentView: View {
    
    @EnvironmentObject var deviceModel: DeviceModel
    
    let timer = Timer.publish(every: 0.02, on: .main, in: .common).autoconnect()

    var body: some View {
        VStack {
            Text("Graphics Simulator")
                .padding()
            Image(nsImage: deviceModel.image).frame(width: 128, height: 128).padding(60)
        }.background(KeyEventHandling()).onReceive(timer){ _ in
            tick()
        }
    }
    
    func tick() {
        let tick_event = fd_event_get_identifier("fd_ux.tick")
        fd_event_set(tick_event)
        fd_event_process()
        if simulateSSD1327.modified {
            simulateSSD1327.modified = false
            let image = NSImage(size: NSSize(width: CGFloat(128), height: CGFloat(128)))
            image.lockFocus()
            simulateSSD1327.image.draw(at: NSPoint(x: 0, y: 0), from: NSRect(x: 0, y: 0, width: 128, height: 128), operation: .sourceOver, fraction: 1)
            image.unlockFocus()
            deviceModel.setScreen(image: image)
        }
    }

}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
