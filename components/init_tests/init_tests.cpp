// c++ includes

#include <iostream>
#include <RadioLib.h>

// custom cpp libraries
#include "rf_comms.h"
#include "EspHal.h"

// extern c for 'c' includes
#ifdef __cplusplus
extern "C" {
#endif

    // standard includes needed
    #include <stdio.h>
    #include <string.h>
    #include "init_tests.h"

    // includes needed for ESP32
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "driver/gpio.h"
    #include "driver/spi_master.h"
    #include "driver/uart.h"

    // driver includes needed
    #include "u8g2.h"
    #include "u8g2_esp32_hal.h"
    #include "esp_camera.h"

    // custom includes needed for inits
    #include "camera.h"
    #include "GUI_drivers.h"
    #include "sd_card.h"
    #include "wifi_comms.h"

#ifdef __cplusplus
}
#endif


// run the init code for all hardware, returns a ESP_OK if all successful
esp_err_t init_devices()
{
    // checking display init
    if ( init_display() != ESP_OK )
    {
        return ESP_FAIL;
    }
    // checking radio init
    if ( init_radio() != ESP_OK )
    {
        return ESP_FAIL;
    }
    // checking camera init
    if ( init_camera() != ESP_OK )
    {
        return ESP_FAIL;
    }
    // checking wifi init
    if ( init_wifi_comms() != ESP_OK )
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}









