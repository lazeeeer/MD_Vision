#include <stdio.h>
#include <string.h>
#include "GUI_drivers.h"

// including the FreeRTOS task libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"

// including radio drivers to get number of messages available
#include "rf_comms.h"

//including the u8g2 and u8g2_hal libs
#include "u8g2.h"
#include "u8g2_esp32_hal.h"

// Defines needed for u8g2 SPI communication
#define SPI_MOSI_PIN    23
#define SPI_MISO_PIN    19
#define SPI_SCK_PIN     18
#define SPI_CS_PIN      0  // this if for DISPLAY only
#define SPI_RESET_PIN   2  // this is for DISPLAY only 

#define PIN_NUM_DC      21
#define PIN_NUM_BCKL    5

#define DISP_BUTTON     2   // GPIO display button is connected to
#define DEBOUNCE_DELAY  50  // delay in ms for debouncing check

#define RADIOLIB_ERR_NONE 0 // define used to know if we got error from radio functions

// Defines for battery capacity meaurment circuits
#define ADC_CHANNEL ADC1_CHANNEL_6      // associated with pin 34 on the board
#define ADC_WIDTH   ADC_WIDTH_BIT_12
#define ADC_ATTEN   ADC_ATTEN_DB_0
#define BATTERY_MIN 3.0
#define BATTERY_MAX 4.3



// ==== Static items for controlling display ========== //

// making a u8g2 object for our main display
static u8g2_t mainDisp;

// define the queue from GUI_drivers.h
QueueHandle_t displayQueue;

// variable for holding and update the measured battery capacity
static float capacity;


// ==== List of main wrapper functions editing display ========== //

// TODO: TEST THE INTI WITH NEW PIN DEFINES
esp_err_t init_display()
{
    //calling default hardware abstaction layer for esp32
    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;

    //passing all pins needed for SPI comms and HAL
    u8g2_esp32_hal.clk   = SPI_SCK_PIN;
    u8g2_esp32_hal.mosi  = SPI_MOSI_PIN;
    u8g2_esp32_hal.cs    = SPI_CS_PIN;
    u8g2_esp32_hal.dc    = PIN_NUM_DC;
    u8g2_esp32_hal.reset = SPI_RESET_PIN;

    //initializin the HAL
    u8g2_esp32_hal_init(u8g2_esp32_hal);

    // using noname0_f -> gives a full frame buffer with 10124 bytes
    u8g2_Setup_ssd1309_128x64_noname0_f(&mainDisp, U8G2_R2, u8g2_esp32_spi_byte_cb, u8g2_esp32_gpio_and_delay_cb);

    // initing the queue and giving it a size of 10
    displayQueue = xQueueCreate(10, sizeof(display_msg_package_t));

    // calling init commands to turn on and clear display
    u8g2_InitDisplay(&mainDisp);
    u8g2_SetPowerSave(&mainDisp, 0);    // turning the display on
    u8g2_ClearDisplay(&mainDisp);

    // setting the main HUD after initialization;
    display_main_hud();
    
    return ESP_OK;
}

// function to clear the ENTIRE display
void clear_disp()
{
    u8g2_ClearBuffer(&mainDisp);
    u8g2_SendBuffer(&mainDisp);
}


// function to write the main HUD of the display onto the frame buffer
// series of commands to make the main display HUD and write the buffer ...
void display_main_hud(void)
{
    // clear curr buffer and set font
    //u8g2_ClearBuffer(&mainDisp);
    u8g2_SetFont(&mainDisp, u8g2_font_5x8_tr);
    
    // write logged-in user on left side of display
    u8g2_DrawStr(&mainDisp, 0, 10, "Main HUD text");

    // draw small notification dot
    u8g2_DrawDisc(&mainDisp, 124, 59, 2, U8G2_DRAW_ALL);

    u8g2_SendBuffer(&mainDisp);
}


// function to clear JUST the middle text 'area'
void display_clear_msg_text()
{
    // draw a 'clear colour' box on top of the text area
    u8g2_SetDrawColor(&mainDisp, 0);
    u8g2_DrawBox(&mainDisp, 14, 10, 100, 80);
    // send editted buffer to display
    u8g2_SendBuffer(&mainDisp);
}



// simple function to update message notification on display
void display_update_notif()
{
    // checking number of available messages in the RF69 module's memory
    if ( 1 > 0 )
    {
        // code to add-in notif symbol from display
        u8g2_SetDrawColor(&mainDisp, 1);
        u8g2_DrawDisc(&mainDisp, 124, 59, 2, U8G2_DRAW_ALL);
    }
    else {
        // code to remove notif symbol from display
        u8g2_SetDrawColor(&mainDisp, 0);
        u8g2_DrawDisc(&mainDisp, 124, 59, 2, U8G2_DRAW_ALL);
    }
}


// simple function to read-in the battery voltage and update the display
void display_update_battery()
{
    int adc_reading = adc1_get_raw(ADC_CHANNEL);
    //uint32_t read_voltage = 0b0;
    printf("Raw ADC reading was: %d\n", adc_reading);

    // calculating percentage from ADC read
    if ( adc_reading >=  BATTERY_MIN )
    {
        capacity = ((float)(adc_reading - BATTERY_MIN) / (BATTERY_MAX - BATTERY_MIN) ) * 100;
    }
    else
    {
        printf("could not measure batter for some reason...\n");
    }

    // TODO: write function to display battery % in top-left corner
    // ...
}


// write a simple verticle line to make sure display buffer has enough allocated memory
void test_pixels()
{
    u8g2_ClearBuffer(&mainDisp);  // Clear the buffer
    
    // Draw pixels across the height of the display
    for (int y = 0; y < 64; y++)  // Adjust to 64 if your display is 128x64
    {
        u8g2_DrawPixel(&mainDisp, 10, y);  // Draw a vertical line at x=10
    }

    u8g2_SendBuffer(&mainDisp);  // Send the buffer to the display
}



// TODO: HAVE FUNCTION HANDLE THE MSG_PACKAGE ISNTEAD TO GET THE MSG FLAG AS WELL
void write_to_disp(const char* str)
{
    u8g2_SetFont(&mainDisp, u8g2_font_5x8_tr);
    u8g2_SetDrawColor(&mainDisp, 1);

    const int line_char_len = 20;

    int currMsgLen = strlen(str);
    //printf("msg length: %d\n", currMsgLen);   // debug

    if (currMsgLen > line_char_len)
    {
        // num of lines needed to display msg based on length
        int splitLines = (currMsgLen / line_char_len)+1;

        for (int i=0; i < splitLines; i++)
        {
            char *substring = (char*)malloc( (line_char_len+1) * sizeof(char) );
            strncpy(substring, &str[i * line_char_len], line_char_len);
            substring[line_char_len] = '\0';

            printf("%s\n", substring);

            u8g2_DrawStr(&mainDisp, 14, (i*10) + 20 , substring);

        }
        // sending buffer after addding al lines to it
        u8g2_SendBuffer(&mainDisp);

    }
    else {  //case of only one line needed
        u8g2_DrawStr(&mainDisp, 14, 20, str);
        u8g2_SendBuffer(&mainDisp);
    }

}




// Main display control loop to run in the task
void displayLoop(void *params)
{
    char message[64];    // buffer to take in the packets from RF module

   static int processState = 0; 
    /* display loop state machine
        0 - idle,       - wait for message on queue     - go to 1
        1 - displaying, - look for button press         - go to 0
    */

   // setting up the GPIO that the button is connected to 
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DISP_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    // main event loop
    while (true)
    {
        display_update_notif();     // check and update notif
        display_update_battery();   // check and update battery

        if (processState == 0)     // idle - waiting for message_available && button_press
        {
            // Check pressed button AND messages avaialable
            // NOTE: button press is debounced in hardware with RC circuit
            if ( (gpio_get_level(DISP_BUTTON) == 0) && (get_numMessages() > 0))
            {
                printf("REACHED DEBOUNCE PASS AND MESSAGES IN QUEUE...\n");

                if ( get_message(&message, sizeof(message)) == RADIOLIB_ERR_NONE )
                {
                    // successfully filled message buffer
                    write_to_disp(message);
                }
                // changing to next state that waits to clear message
                processState = 1;
            }
            // end of state...
        }
        else if (processState == 1)    // displaying message, waiting for next button input
        {
            // checking same button state 
            if ( gpio_get_level(DISP_BUTTON) == 0 )
            {
                display_clear_msg_text();

                // handling when the button is held-down
                if ( gpio_get_level(DISP_BUTTON) == 1)
                {
                    processState = 0;   // switching state back to idle
                }
            }
        }

        // yield to scheduler for a bit between checks
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}







