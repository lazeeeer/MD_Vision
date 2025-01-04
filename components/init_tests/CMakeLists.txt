
# USE THIS AS AN EXAMPLE OF HOW TO ADD THE CMakeLists.txt FILE FOR NEW CUSTOM COMPONENTS
# ADDING IN A "COMPONENT" FOLDER WITH CUSTOM CODE IN HERE MEANS THE "MAIN" FOLDER NEEDS TO "REQUIRE" THE NAME OF THE COMPONENT FOLDER IN IT'S OWN CMakeLists.txt FILE

idf_component_register(SRCS "init_tests.cpp"
                        INCLUDE_DIRS "."
                        #REQUIRES driver esp32-camera u8g2 u8g2_hal camera sd_card wifi_comms RadioLib rf_comms EspHal GUI_drivers SPI_interface
                        REQUIRES esp_timer test_component driver u8g2 u8g2_hal GUI_drivers SPI_interface vfs fatfs sdmmc esp_driver_sdmmc esp32-camera camera sd_card wifi_comms jsmn RadioLib rf_comms EspHal
                    )


