//
//  Graphics_ConverterApp.swift
//  Graphics Converter
//
//  Created by Denis Bohm on 3/8/21.
//

import SwiftUI

@main
struct Graphics_ConverterApp: App {
    
    @StateObject private var model = ConverterModel()
    
    var body: some Scene {
        WindowGroup {
            ContentView().environmentObject(model)
        }
    }

}
