//
//  ConverterModel.swift
//  Graphics Converter
//
//  Created by Denis Bohm on 3/8/21.
//

import Cocoa

class ConverterModel: ObservableObject {
    
    @Published var image: NSImage = NSImage(named: NSImage.advancedName)!
    
    struct Glyph {
        let character: Character
        let width: Int
        let height: Int
        let originX: Int
        let originY: Int
        let advance: Int
        let data: Data
        let image: NSImage
    }
    
    init() {
        let prefix = "fd_"
        let height:Float = 6
        let depth = 4
        let cFontName = "B612-Regular"
        let cFontSymbol = "b612_regular_6"
        let fileName = Bundle.main.path(forResource: "B612-Regular", ofType: "ttf")!
        let characters =
            "!\"#$%&'()*+,-./" +
            "0123456789" +
            ":;<=>?@" +
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ" +
            "[\\]^_`" +
            "abcdefghijklmnopqrstuvwxyz" +
            "{|}~"
        
        let freeType = try! FreeType()
        let fontConverter = FontConverter(freeType: freeType)
        fontConverter.setFont(fileName: fileName, height: height)
        fontConverter.setCharacter(character: Character(" "))
        let advance = freeType.getGlyphAdvance()
        
        var glyphs = [Glyph]()
        for character in characters {
            fontConverter.setCharacter(character: character)
            let data = fontConverter.getGlyphData(depth: depth)
            let image = fontConverter.getGlyphImage()
            let glyph = Glyph(character: character, width: freeType.getGlyphBitmapWidth(), height: freeType.getGlyphBitmapHeight(), originX: freeType.getGlyphBitmapOriginX(), originY: freeType.getGlyphBitmapOriginY(), advance: freeType.getGlyphAdvance(), data: data, image: image)
            glyphs.append(glyph)
            self.image = image
        }

        var h = "#ifndef \(prefix)font_\(cFontSymbol)_h\n"
        h += "#define \(prefix)font_\(cFontSymbol)_h\n"
        h += "\n"
        h += "#include \"\(prefix)graphics.h\"\n"
        h += "\n"
        h += "extern const \(prefix)graphics_font_t \(prefix)font_\(cFontSymbol);\n"
        h += "\n"
        h += "#endif\n"
        print(h)
        
        var c = "#include \"\(prefix)font_\(cFontSymbol).h\"\n"
        c += "\n"
        
        for glyph in glyphs {
            let code = Int(glyph.character.asciiValue!)
            let codeString = String(format: "%04x", code)
            c += "const uint8_t \(prefix)font_\(cFontSymbol)_data_\(codeString)[] = {\n"
            c += getDataCString(data: glyph.data)
            c += "\n"
            c += "};\n"
            c += "\n"
        }
        
        c += "const \(prefix)graphics_glyph_t \(prefix)font_\(cFontSymbol)_glyphs[] = {\n"
        
        for glyph in glyphs {
            let code = Int(glyph.character.asciiValue!)
            let codeString = String(format: "%04x", code)
            c += "    {\n"
            c += "        .character = 0x\(codeString),\n"
            c += "        .advance = \(glyph.advance),\n"
            c += "        .bitmap = {\n"
            c += "            .width = \(glyph.width),\n"
            c += "            .height = \(glyph.height),\n"
            c += "            .depth = \(depth),\n"
            c += "            .origin = { .x = \(glyph.originX), .y = \(glyph.originY)},\n"
            c += "            .data = \(prefix)font_\(cFontSymbol)_data_\(codeString),\n"
            c += "        },\n"
            c += "    },\n"
        }
        
        c += "};\n"
        c += "\n"
        c += "const \(prefix)graphics_font_t \(prefix)font_\(cFontSymbol) = {\n"
        c += "    .name = \"\(cFontName)\",\n"
        c += "    .height = \(String(format: "%0.1f", height))f,\n"
        c += "    .advance = \(advance),\n"
        c += "    .ascent = 0,\n"
        c += "    .glyph_count = \(glyphs.count),\n"
        c += "    .glyphs = \(prefix)font_\(cFontSymbol)_glyphs,\n"
        c += "};\n"
        print(c)
    }
    
    func getDataCString(data: Data) -> String {
        var string = "   "
        var column = string.count
        for byte in data {
            if column + 6 > 80 {
                string += "\n   "
                column = 3
            }
            string += String(format: " 0x%02x,", byte)
            column += 6
        }
        return string
    }
    
}
