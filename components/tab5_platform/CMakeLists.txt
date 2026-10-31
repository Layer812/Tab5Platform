idf_component_register(
    SRCS "src/tab5_platform.cpp" "src/tab5_input_usb.cpp" "src/tab5_geometry.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES M5Unified M5GFX freertos esp_timer esp_hw_support heap
                  log fatfs sdmmc esp_driver_sdmmc esp_driver_gpio
                  usb usb_host_hid
)
