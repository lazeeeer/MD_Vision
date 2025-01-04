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


// main function for calling series of tests/checks
void init_checks(void)
{

}


// ---- INIT functions for different components of the device ----

void init_display_device(void)
{
    // initing the display and making it enter default state and HUD
    init_display();
    clear_disp();
    display_main_hud();
}

void init_wifi_device(void)
{
    // this will init wifi drivers AND attempt to CONNECT to a wifi station, not make a request
    // making HTTP connections is a different function call
    // CHECK AND HANDLE CONNECTION FAILED?????
    init_wifi_comms();
}

void init_pager_device()
{
    // turn the radio on and set it into a LISTENING PAGER MODE
    init_radio();
}

// ---- POST tests for checking hardware ----







