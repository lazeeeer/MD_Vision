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

    // using noname0_f -> gives a full frame buffer with 10124 bytes
    u8g2_Setup_ssd1309_128x64_noname0_f(&mainDisp, U8G2_R0, u8g2_esp32_spi_byte_cb, u8g2_esp32_gpio_and_delay_cb);

    // calling init commands to turn on and clear display
    u8g2_InitDisplay(&mainDisp);
    u8g2_SetPowerSave(&mainDisp, 0);    // turning the display on
    u8g2_ClearDisplay(&mainDisp);
    
}

void clear_disp()
{
    u8g2_ClearDisplay(&mainDisp);
}


// display a string on multiple lines, keeping words intact where possible
void printwords(const char *msg, int xloc, int yloc) {

   int dspwidth = u8g2_GetDisplayWidth(&mainDisp);                        // display width in pixels
   int strwidth = 0;                         // string width in pixels
   char glyph[2]; glyph[1] = 0;

   for (const char *ptr = msg, *lastblank = NULL; *ptr; ++ptr) {
      while (xloc == 0 && *msg == ' ')
         if (ptr == msg++) ++ptr;                        // skip blanks at the left edge

      glyph[0] = *ptr;
      strwidth += u8g2_GetStrWidth(&mainDisp, glyph);                        // accumulate the pixel width
      if (*ptr == ' ')  lastblank = ptr;                        // remember where the last blank was
      else ++strwidth;                        // non-blanks will be separated by one additional pixel

      if (xloc + strwidth > dspwidth) {                        // if we ran past the right edge of the display
         int starting_xloc = xloc;
         while (msg < (lastblank ? lastblank : ptr)) {                        // print to the last blank, or a full line
            glyph[0] = *msg++;
            xloc += u8g2_DrawStr(&mainDisp, xloc, yloc, glyph); 
         }

         strwidth -= xloc - starting_xloc;                        // account for what we printed
         yloc += u8g2_GetMaxCharHeight(&mainDisp);                        // advance to the next line
         xloc = 0; lastblank = NULL;
      }
   }
   while (*msg) {                        // print any characters left over
      glyph[0] = *msg++;
      xloc += u8g2_DrawStr(&mainDisp, xloc, yloc, glyph);
   }
}


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



// TODO : HANDLING MULTI LINE MESSAGES
void write_to_disp(int x, int y, const char* str)
{
    u8g2_ClearBuffer(&mainDisp);

    const int line_char_len = 20;
    const int num_of_lines = 5;
    const int line_spacing = 10;

    int currMsgLen = strlen(str);
    printf("msg length: %d\n", currMsgLen);

    if (currMsgLen > line_char_len)
    {
        u8g2_SetFont(&mainDisp, u8g2_font_ncenB08_tr);

        // num of lines needed to display msg based on length
        int splitLines = (currMsgLen / line_char_len)+1;

        for (int i=0; i < splitLines; i++)
        {
            char *substring = (char*)malloc( (line_char_len+1) * sizeof(char) );
            strncpy(substring, &str[i * line_char_len], line_char_len);
            substring[line_char_len] = '\0';

            printf("%s\n", substring);


            u8g2_DrawStr(&mainDisp, 1, (i*10) + 20 , substring);

        }
        // sending buffer after addding al lines to it
        u8g2_SendBuffer(&mainDisp);

    }
    else {  //case of only one line needed
        u8g2_SetFont(&mainDisp, u8g2_font_ncenB08_tr);
        u8g2_DrawStr(&mainDisp, x, y, str);
        u8g2_SendBuffer(&mainDisp);
    }

}

// more to come...



