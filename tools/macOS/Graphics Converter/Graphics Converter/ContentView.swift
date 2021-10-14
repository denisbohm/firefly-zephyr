//
//  ContentView.swift
//  Graphics Converter
//
//  Created by Denis Bohm on 3/8/21.
//

import SwiftUI

struct ContentView: View {
    
    @EnvironmentObject var model: ConverterModel
    
    var body: some View {
        VStack {
            Image(nsImage: self.model.image)
            Text("Hello, world!")
        }.frame(maxWidth: .infinity, maxHeight: .infinity)
    }
    
}

struct ContentView_Previews: PreviewProvider {
    
    static var previews: some View {
        ContentView()
    }
    
}
