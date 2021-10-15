//
//  ContentView.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/12/21.
//

import SwiftUI

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
