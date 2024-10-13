idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES lvgl test_component driver u8g2 u8g2_hal GUI_drivers SPI_interface
                )
