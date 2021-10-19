//
//  Graphics_SimulatorApp.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/12/21.
//

import SwiftUI

@main
struct Graphics_SimulatorApp: App {
    
    static let buttons_gpios = [
        fd_gpio_t(port: 0, pin: 9),
        fd_gpio_t(port: 1, pin: 10),
    ]
    
    let deviceModel = DeviceModel()

    var body: some Scene {
        WindowGroup {
            ContentView().environmentObject(deviceModel)
        }
    }
    
    init() {
        fd_watch_initialize();

        deviceModel.setScreen(image: simulateSSD1327.image)
    }

}
