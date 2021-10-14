//
//  GraphicsModel.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 10/12/21.
//

import Foundation
import SwiftUI

class DeviceModel: ObservableObject {

    @Published private(set) var image = NSImage()

    func setScreen(image: NSImage) {
        self.image = image
    }

}
