set(OVERLAY_CONFIG Kconfig)

zephyr_include_directories(include)

if(CONFIG_FIREFLY_SUBSYS_GRAPHICS)

zephyr_sources(src/fd_bitmap_battery.c)
zephyr_sources(src/fd_bitmap_charging.c)
zephyr_sources(src/fd_bitmap_powered.c)
zephyr_sources(src/fd_canvas.c)
zephyr_sources(src/fd_drawing.c)
zephyr_sources(src/fd_drawing_battery.c)
zephyr_sources(src/fd_drawing_icon.c)
zephyr_sources(src/fd_drawing_image.c)
zephyr_sources(src/fd_drawing_rectangle.c)
zephyr_sources(src/fd_drawing_text.c)
zephyr_sources(src/fd_font_b612_regular_6.c)
zephyr_sources(src/fd_font_b612_regular_12.c)
zephyr_sources(src/fd_font_b612_regular_14.c)
zephyr_sources(src/fd_font_b612_regular_16.c)
zephyr_sources(src/fd_font_merriweather_sans_regular_6.c)
if(CONFIG_FIREFLY_SUBSYS_GRAPHICS_CST816S)
zephyr_sources(src/fd_cst816s.c)
endif()
if(CONFIG_FIREFLY_SUBSYS_GRAPHICS_GC9A01)
zephyr_sources(src/fd_gc9a01.c)
zephyr_sources(src/fd_spi_display_nrf5.c)
endif()
zephyr_sources(src/fd_graphics.c)
zephyr_sources(src/fd_graphics_color_16bit.c)
zephyr_sources(src/fd_graphics_color_8bit.c)
zephyr_sources(src/fd_graphics_gray_4bit.c)
if(CONFIG_FIREFLY_SUBSYS_GRAPHICS_ILI9341)
zephyr_sources(src/fd_graphics_ili9341.c)
zephyr_sources(src/fd_ili9341.c)
zephyr_sources(src/fd_ili9341_bus_8bit.c)
endif()
if(CONFIG_FIREFLY_SUBSYS_GRAPHICS_SSD1327)
zephyr_sources(src/fd_graphics_ssd1327.c)
zephyr_sources(src/fd_ssd1327.c)
zephyr_sources(src/fd_ssd1327_bus_4_wire_spi_nrf5.c)
endif()
if(CONFIG_FIREFLY_SUBSYS_GRAPHICS_SSD1331)
zephyr_sources(src/fd_ssd1331.c)
zephyr_sources(src/fd_spi_display_nrf5.c)
endif()
if(CONFIG_FIREFLY_SUBSYS_GRAPHICS_TXDC0421A1)
zephyr_sources(src/fd_graphics_txdc042a1.c)
zephyr_sources(src/fd_txdc042a1.c)
zephyr_sources(src/fd_txdc042a1_bus_4_wire_spi_nrf5.c)
endif()
zephyr_sources(src/fd_view.c)
zephyr_sources(src/fd_view_battery.c)
zephyr_sources(src/fd_view_icon.c)
zephyr_sources(src/fd_view_image.c)
zephyr_sources(src/fd_view_rectangle.c)
zephyr_sources(src/fd_view_text.c)

endif()
