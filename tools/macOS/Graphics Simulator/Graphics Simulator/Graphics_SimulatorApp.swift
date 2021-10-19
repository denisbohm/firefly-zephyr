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
        fd_watch_initialize();

        deviceModel.setScreen(image: simulateSSD1327.image)
    }

}
