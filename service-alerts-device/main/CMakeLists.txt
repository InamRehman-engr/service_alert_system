idf_component_register(SRCS "rgb_lcd_example_main.c" "lvgl_demo_ui.c" "alert_ui/ui.c" "alert_ui/logo.c"
                       INCLUDE_DIRS "." "alert_ui"
                       REQUIRES lvgl driver log esp_lcd json iotcore)

# Define LV_LVGL_H_INCLUDE_SIMPLE for font files
target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_LVGL_H_INCLUDE_SIMPLE)
