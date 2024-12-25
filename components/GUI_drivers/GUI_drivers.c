#include <stdio.h>
#include <string.h>
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


// ==== List of wrapper functions editing display ========== //

// making a u8g2 object for our main display
static u8g2_t mainDisp;

// define the queue from GUI_drivers.h
QueueHandle_t displayQueue;


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

    // using noname0_f -> gives a full frame buffer with 10124 bytes
    u8g2_Setup_ssd1309_128x64_noname0_f(&mainDisp, U8G2_R0, u8g2_esp32_spi_byte_cb, u8g2_esp32_gpio_and_delay_cb);

    // initing the queue and giving it a size of 10
    displayQueue = xQueueCreate(10, sizeof(display_msg_package_t));

    // calling init commands to turn on and clear display
    u8g2_InitDisplay(&mainDisp);
    u8g2_SetPowerSave(&mainDisp, 0);    // turning the display on
    u8g2_ClearDisplay(&mainDisp);
    
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
    if ( uxQueueGetQueueNumber(displayQueue) > 0 )
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

// TODO : GET BUTTON STATE CHECKING WORKING AND TESTED
// Main display control loop to run in the task
void displayLoop(void *params)
{
   static int state = 0; 
    /* display loop state machine
        0 - idle,       - wait for message on queue     - go to 1
        1 - displaying, - look for button press         - go to 0
    */

    // hold the msg struct incoming from the queue
    display_msg_package_t received_msg;

    // main event loop
    while (true)
    {
        // always check and update msg notification icon
        void display_update_notif();

        if (state == 0)     // idle and waiting for button input
        {
            // Check if we have a new message to update display
            if ( uxQueueGetQueueNumber > 0 && true /* checking button state variable */ ) {

                xQueueReceive(displayQueue, &received_msg, portMAX_DELAY);
                write_to_disp(received_msg.message);

                // set button state back down to zero

                // change to next state 
                state = 1;
            }
            //one of prev conditions not true... continue

        }
        else if (state == 1)    // displaying message, waiting for next button input
        {
            // search for button input to indicate done reading
            if ( true /* code that looks for button input */ )
            {
                /* */                       // set button state back down to zero
                display_clear_msg_text();   // clear msg text area on display
                state = 0;                  // set state back to idle + wait for input
            }
        }

        // yield to scheduler for a bit between checks
        vTaskDelay(250/portTICK_PERIOD_MS);
    }

}







