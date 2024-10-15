#include <stdio.h>
#include "GUI_drivers.h"

//including the u8g2 and u8g2_hal libs
#include "u8g2.h"
#include "u8g2_esp32_hal.h"

// Defines needed for u8g2 SPI communication
#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22
#define PIN_NUM_DC   21
#define PIN_NUM_RST  18
#define PIN_NUM_BCKL 5

// making a u8g2 object for our main display
static u8g2_t mainDisp;

// ==== List of main wrapper functions editing display ========== //

void init_display()
{
    //calling default hardware abstaction layer for esp32
    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;

    //passing all pins needed for SPI comms and HAL
    u8g2_esp32_hal.clk   = PIN_NUM_CLK;
    u8g2_esp32_hal.mosi  = PIN_NUM_MOSI;
    u8g2_esp32_hal.cs    = PIN_NUM_CS;
    u8g2_esp32_hal.dc    = PIN_NUM_DC;
    u8g2_esp32_hal.reset = PIN_NUM_RST;

    //initializin the HAL
    u8g2_esp32_hal_init(u8g2_esp32_hal);

    // init'ing HAL to use our display driver board
    u8g2_Setup_ssd1309_128x64_noname0_2(&mainDisp, U8G2_R0, u8g2_esp32_spi_byte_cb, u8g2_esp32_gpio_and_delay_cb);
    u8g2_InitDisplay(&mainDisp);
    u8g2_SetPowerSave(&mainDisp, 0);    // turning the display on
    u8g2_ClearDisplay(&mainDisp);       // clearing the display
    
}

void clear_disp()
{
    u8g2_ClearDisplay(&mainDisp);
}

void write_to_disp(int x, int y, const char* str)
{
    // THIS IS TEMP - TODO
    u8g2_SetFont(&mainDisp, u8g2_font_ncenB08_tr);

    u8g2_DrawStr(&mainDisp, x, y, str);
    u8g2_SendBuffer(&mainDisp);
}

// more to come...



