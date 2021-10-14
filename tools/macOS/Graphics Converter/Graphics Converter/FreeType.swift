//
//  FreeType.swift
//  Graphics Converter
//
//  Created by Denis Bohm on 3/8/21.
//

import Foundation

class FreeType {
    
    enum FreeTypeError: Error {
        case failure(status: FT_Error)
        case general
    }
    
    var ft_library: FT_Library?
    var ft_face: FT_Face?
    var character: Int = 0
    var ft_glyph: UnsafeMutablePointer<FT_GlyphSlotRec_>?

    init() throws {
        let status = FT_Init_FreeType(&ft_library)
        if status != FT_Err_Ok {
            throw FreeTypeError.failure(status: status)
        }
    }
    
    func setFont(fileName: String) throws {
        let status = FT_New_Face(ft_library, fileName, 0, &ft_face)
        if status != FT_Err_Ok {
            throw FreeTypeError.failure(status: status)
        }
    }
    
    func setHeight(height: Float) throws {
        var size_request = FT_Size_RequestRec_(
            type: FT_SIZE_REQUEST_TYPE_NOMINAL,
            width: 0,
            height: Int(height * 64.0), // size unit is 1/64th point
            horiResolution: 120,
            vertResolution: 120
        )
        let status = FT_Request_Size(ft_face, &size_request)
        if status != FT_Err_Ok {
            throw FreeTypeError.failure(status: status)
        }
    }
    
    func setCharacter(character: Int) throws {
        let glyphIndex = FT_Get_Char_Index(ft_face, FT_ULong(character))
        if glyphIndex == 0 {
            throw FreeTypeError.general
        }
        var status = FT_Load_Glyph(ft_face, glyphIndex, FT_LOAD_DEFAULT)
        if status != FT_Err_Ok {
            throw FreeTypeError.failure(status: status)
        }
        guard let glyph = ft_face?.pointee.glyph else {
            throw FreeTypeError.general
        }
        if glyph.pointee.format != FT_GLYPH_FORMAT_BITMAP {
            status = FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL)
            if status != FT_Err_Ok {
                throw FreeTypeError.failure(status: status)
            }
        }
        if glyph.pointee.format != FT_GLYPH_FORMAT_BITMAP {
            throw FreeTypeError.general
        }
        self.character = character
        self.ft_glyph = glyph
    }
    
    func getGlyphBitmapWidth() -> Int {
        return Int(ft_glyph!.pointee.bitmap.width)
    }
    
    func getGlyphBitmapHeight() -> Int {
        return Int(ft_glyph!.pointee.bitmap.rows)
    }
    
    func getGlyphBitmapOriginX() -> Int {
        return Int(-ft_glyph!.pointee.bitmap_left)
    }
    
    func getGlyphBitmapOriginY() -> Int {
        return Int(ft_glyph!.pointee.bitmap_top)
    }
    
    func getGlyphBounds() -> CGRect {
        let bitmap = ft_glyph!.pointee.bitmap
        var bounds = CGRect()
        bounds.origin.x = CGFloat(ft_glyph!.pointee.bitmap_left)
        bounds.origin.y = CGFloat(ft_glyph!.pointee.bitmap_top)
        bounds.size.width = CGFloat(bitmap.width)
        bounds.size.height = CGFloat(bitmap.rows)
        return bounds
    }
    
    func getGlyphAdvance() -> Int {
        return Int(ft_glyph!.pointee.advance.x >> 6)
    }
    
    func getPixel(x: Int, y: Int) throws -> Double {
        let bitmap = ft_glyph!.pointee.bitmap
        guard let buffer = bitmap.buffer else {
            throw FreeTypeError.general
        }
        let byte = buffer[y * Int(bitmap.pitch) + x]
        let value = Double(byte) / 255.0
        return value
    }
    
    func getKerning(character: Int, successorCharacter: Int) -> Int {
        let glyphIndex = FT_Get_Char_Index(ft_face, FT_ULong(character))
        if glyphIndex == 0 {
            return 0
        }
        let successorGlyphIndex = FT_Get_Char_Index(ft_face, FT_ULong(successorCharacter))
        if successorGlyphIndex == 0 {
            return 0
        }
        var delta = FT_Vector(x: 0, y: 0)
        let status = FT_Get_Kerning(ft_face, glyphIndex, successorGlyphIndex, FT_KERNING_DEFAULT.rawValue, &delta);
        if status != FT_Err_Ok {
            return 0
        }
        return delta.x >> 6
    }
    
}
