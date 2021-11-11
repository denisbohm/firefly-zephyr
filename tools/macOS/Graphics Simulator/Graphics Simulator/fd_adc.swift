//
//  fd_adc.swift
//  Graphics Simulator
//
//  Created by Denis Bohm on 11/11/21.
//

import Foundation

// void fd_adc_initialize(void);
@_cdecl("fd_adc_initialize")
func fd_adc_initialize() {
}

// float fd_adc_convert(int channel, float max_voltage);
@_cdecl("fd_adc_convert")
func fd_adc_convert(channel: Int, max_voltage: Float) -> Float {
    return 0
}
