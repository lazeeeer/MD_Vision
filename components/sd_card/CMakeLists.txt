idf_component_register(SRCS "sd_card.c"
                        INCLUDE_DIRS "."
                        REQUIRES driver sdmmc esp_driver_sdmmc vfs fatfs esp32-camera
                    )
