idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES esp_timer test_component driver u8g2 u8g2_hal GUI_drivers SPI_interface vfs fatfs sdmmc esp_driver_sdmmc esp32-camera camera sd_card wifi_comms jsmn RadioLib rf_comms EspHal sync_objects cJSON
                )
