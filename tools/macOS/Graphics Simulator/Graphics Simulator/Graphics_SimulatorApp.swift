//
//  Graphics_SimulatorApp.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/12/21.
//

import SwiftUI

@main
struct Graphics_SimulatorApp: App {
    
    let deviceModel = DeviceModel()

    var body: some Scene {
        WindowGroup {
            ContentView().environmentObject(deviceModel)
        }
    }
    
    init() {
        fd_gpio_initialize()
        fd_event_initialize()
        fd_graphics_ssd1327_initialize();
        fd_event_initialize();
        
        let graphics = fd_graphics_ssd1327_get()
        withUnsafePointer(to: fd_font_b612_regular_6) { font in
            fd_graphics_set_font(graphics, font)
        }
        fd_graphics_write_string(graphics, 0, 20, "hi")

        deviceModel.setScreen(image: simulateSSD1327.image)
    }

}
