//
//  fd_pwm.swift
//  Itamar Loop Simulator
//
//  Created by Denis Bohm on 12/20/21.
//

import Foundation

//void fd_pwm_initialize(const fd_pwm_module_t *modules, uint32_t module_count, const fd_pwm_channel_t *channels, uint32_t channel_count);
@_cdecl("fd_pwm_initialize")
func fd_pwm_initialize(modules: fd_pwm_module_t, module_count: UInt32, channels: fd_pwm_channel_t, channel_count: UInt32) {
}

//void fd_pwm_module_enable(const fd_pwm_module_t *module);
@_cdecl("fd_pwm_module_enable")
func fd_pwm_module_enable(module: fd_pwm_module_t) {
}

//void fd_pwm_module_disable(const fd_pwm_module_t *module);
@_cdecl("fd_pwm_module_disable")
func fd_pwm_module_disable(module: fd_pwm_module_t) {
}

//void fd_pwm_channel_start(const fd_pwm_channel_t *channel, float duty_cycle);
@_cdecl("fd_pwm_channel_start")
func fd_pwm_channel_start(channel: fd_pwm_channel_t, duty_cycle: Float) {
}

//void fd_pwm_channel_stop(const fd_pwm_channel_t *channel);//
@_cdecl("fd_pwm_channel_stop")
func fd_pwm_channel_stop(channel: fd_pwm_channel_t) {
}

//bool fd_pwm_channel_is_running(const fd_pwm_channel_t *channel);
@_cdecl("fd_pwm_channel_is_running")
func fd_pwm_channel_is_running(channel: fd_pwm_channel_t) -> Bool {
    return true
}
