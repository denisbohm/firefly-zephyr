//
//  FontConverter.swift
//  Graphics Converter
//
//  Created by Denis Bohm on 3/8/21.
//

import Cocoa

class FontConverter {
    
    struct Glyph {
        let width: Int
        let height: Int
        let originX: Int
        let originY: Int
        let image: NSImage
    }
    
    let freeType: FreeType
    
    init(freeType: FreeType) {
        self.freeType = freeType
    }
    
    func setFont(fileName: String, height: Float) {
        try! freeType.setFont(fileName: fileName)
        try! freeType.setHeight(height: height)
    }
    
    func setCharacter(character: Character) {
        try! freeType.setCharacter(character: Int(character.asciiValue!))
    }
    
    func getGlyphImage() -> NSImage {
        let width = freeType.getGlyphBitmapWidth()
        let height = freeType.getGlyphBitmapHeight()
        let image = NSImage(size: NSMakeSize(CGFloat(width), CGFloat(height)))
        image.lockFocus()
        
        for x in 0 ..< width {
            for y in 0 ..< height {
                let value = try! freeType.getPixel(x: x, y: height - 1 - y)
                let gray = CGFloat(value)
                let color = NSColor(red: gray, green: gray, blue: gray, alpha: 1.0)
                color.set()
                NSMakeRect(CGFloat(x), CGFloat(y), 1, 1).fill()
            }
        }
        
        image.unlockFocus()
        return image
    }

    func getGlyph() -> Glyph {
        let width = freeType.getGlyphBitmapWidth()
        let height = freeType.getGlyphBitmapHeight()
        let originX = freeType.getGlyphBitmapOriginX()
        let originY = freeType.getGlyphBitmapOriginY()
        let image = getGlyphImage()
        return Glyph(width: width, height: height, originX: originX, originY: originY, image: image)
    }
    
    func getGlyphData(depth: Int) -> Data {
        // for an n-bit depth bitmap (where 1 <= n && n <= 8):
        // bits for x = 0 start at MSB of first byte, followed by bits for x = 1, etc - packed
        // first byte is y = 0, plus stride is y = 1, ...
        // stride is 8 * ceil((width * depth) / 8)
        var data = Data()
        let width = freeType.getGlyphBitmapWidth()
        let height = freeType.getGlyphBitmapHeight()
        let scale = Double((1 << depth) - 1)
        var byte: UInt8 = 0
        var count = 0
        for y in 0 ..< height {
            for x in 0 ..< width {
                let pixel = try! freeType.getPixel(x: x, y: y)
                let value = UInt8(ceil(pixel * scale))
                byte = (byte << depth) | value
                count += depth
                if count >= 8 {
                    data.append(byte)
                    byte = 0
                    count = 0
                }
            }
            if count > 0 {
                byte = byte << (8 - count)
                data.append(byte)
                byte = 0
                count = 0
            }
        }
        return data
    }
    
}
