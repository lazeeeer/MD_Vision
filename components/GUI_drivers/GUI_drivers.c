
#include <stdio.h>
#include <string.h>

// including the FreeRTOS task libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// includes for adc drivers
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"


// including custom driver code
#include "rf_comms.h"
#include "GUI_drivers.h"
#include "sync_objects.h"

//including the u8g2 and u8g2_hal libs
#include "u8g2.h"
#include "u8g2_esp32_hal.h"

// Defines needed for u8g2 SPI communication
#define SPI_MOSI_PIN    36 // was 35 // was 23
#define SPI_MISO_PIN    -1
#define SPI_SCK_PIN     35 // was 37 // was 18
#define SPI_CS_PIN      42  // this if for DISPLAY only
#define SPI_RESET_PIN   1  // this is for DISPLAY only 

#define PIN_NUM_DC      2 // used to be 21

#define DISP_BUTTON     48   // GPIO display button is connected to
#define DEBOUNCE_DELAY  50  // delay in ms for debouncing check

#define RADIOLIB_ERR_NONE 0 // define used to know if we got error from radio functions

#define MSG_CHAR_LEN 256

// Defines for battery capacity meaurment circuits
#define ADC_UNIT        ADC_UNIT_1
#define ADC_CHANNEL     ADC_CHANNEL_6
#define ADC_ATTEN       ADC_ATTEN_DB_11
#define ADC_BITWIDTH    ADC_BITWIDTH_12

static adc_oneshot_unit_handle_t    adc_handle;
static adc_cali_handle_t            adc_cali_handle;
static bool cali_enabled = false;

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

// init the ADC for measuring the battery voltag - TODO: IMPLEMENT BATTERY READING
void init_adc()
{
    // init the adc unit and handle
    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = ADC_UNIT
    };  
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_config, &adc_handle));

    // init the adc channel
    adc_oneshot_chan_cfg_t adc_channel_config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTEN
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &adc_channel_config));
}


// init the display - to be called on startup and used indefinitely
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
    display_clear_msg_text();
    
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
    //u8g2_DrawDisc(&mainDisp, 124, 59, 2, U8G2_DRAW_ALL);

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
    int msgs = uxQueueMessagesWaiting(xMsgBufferQueue);
    // checking number of available messages in the RF69 module's memory
    if ( msgs > 0 )
    {
        // code to add-in notif symbol from display
        u8g2_SetDrawColor(&mainDisp, 1);
        u8g2_DrawDisc(&mainDisp, 124, 59, 2, U8G2_DRAW_ALL);
        u8g2_SendBuffer(&mainDisp);
        //u8g2_DrawStr(&mainDisp, 119, 59, (char)msgs );
    }
    else {
        // code to remove notif symbol from display
        u8g2_SetDrawColor(&mainDisp, 0);
        u8g2_DrawDisc(&mainDisp, 124, 59, 2, U8G2_DRAW_ALL);
        u8g2_SendBuffer(&mainDisp);
        //u8g2_DrawStr(&mainDisp, 119, 59, "0" );
    }
}


#if 0
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
#endif

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


void write_to_disp_temp(const char* str, int timeDly)
{
    write_to_disp(str);
    vTaskDelay(pdMS_TO_TICKS( timeDly * 1000 ));
    display_clear_msg_text();
}


void write_patient_info(display_msg_package_t* patientInfo)
{
    u8g2_SetFont(&mainDisp, u8g2_font_5x8_tr);
    u8g2_SetDrawColor(&mainDisp, 1);

    // displaying patient information 1-by-1
    u8g2_DrawStr(&mainDisp, 14, 20+(0*10), patientInfo->f_name);
    u8g2_DrawStr(&mainDisp, 14, 20+(1*10), patientInfo->l_name);
    u8g2_DrawStr(&mainDisp, 14, 20+(2*10), patientInfo->last_checkup_date);
    u8g2_DrawStr(&mainDisp, 14, 20+(3*10), patientInfo->last_checkup_time);
    u8g2_SendBuffer(&mainDisp);

    // small delay before cleaing the information
    vTaskDelay(pdMS_TO_TICKS(5000));
    display_clear_msg_text();
}


// TODO: HAVE FUNCTION HANDLE THE MSG_PACKAGE ISNTEAD TO GET THE MSG FLAG AS WELL
void write_to_disp(const char* str)
{
    // take display semaphore for safe writing from tasks   
    if ( xSemaphoreTake(xMsgDisplaySem, portMAX_DELAY) == pdTRUE) {

        if (str == NULL){
            printf("string given was null...\n");
            return;
        }

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
                if (substring == NULL) {printf("malloc failed here...\n"); return;}
                strncpy(substring, &str[i * line_char_len], line_char_len);
                substring[line_char_len] = '\0';

                printf("%s\n", substring);

                u8g2_DrawStr(&mainDisp, 14, (i*10) + 20 , substring);
                free(substring);
            }
            // sending buffer after addding al lines to it
            u8g2_SendBuffer(&mainDisp);

        }
        else {  //case of only one line needed
            u8g2_DrawStr(&mainDisp, 14, 20, str);
            u8g2_SendBuffer(&mainDisp);
        }

        xSemaphoreGive(xMsgDisplaySem);
    }
    else {
        // error handling for semaphore take fail...
    }
}




// Main display control loop to run in the task
void displayLoop(void *params)
{
    char message[MSG_CHAR_LEN];    // buffer to take in the packets from RF module
    bool last_state = gpio_get_level(DISP_BUTTON);

    int processState = 0; 
    /* display loop state machine
        0 - idle,       - wait for message on queue     - go to 1
        1 - displaying, - look for button press         - go to 0
    */

    gpio_config_t io_config = {
        .pin_bit_mask = (1ULL << DISP_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_config);

    // main event loop
    for(;;)
    {
        display_update_notif();     // check and update notif
        //display_update_battery();   // check and update battery

        bool current_state = gpio_get_level(DISP_BUTTON);
        //printf("button currently reading: %d\n", current_state);

        if (processState == 0)     // idle - waiting for message_available && button_press
        {
            // Check if button is pressed AND messages avaialable in queue
            // NOTE: button press is debounced in hardware with RC circuit
            if ( (current_state == 0) && (last_state == 1) && ( uxQueueMessagesWaiting(xMsgBufferQueue) > 0) )
            {
                // get message from buffer and put it into message[] by reference
                if ( xQueueReceive( xMsgBufferQueue, &message, portMAX_DELAY) == pdPASS)
                {
                    printf("pulled out the message from buffer: %s\n", message);
                    // write the message to the display then clean the buffer after
                    write_to_disp( message );
                    memset(message, 0, sizeof(message));
                }
                else {
                    printf("Could not get message from xMsgBufferQueue for some reason...\n");
                }
                processState = 1;   //change to next state
            }
        }
        else if (processState == 1)    // displaying message, waiting for next button input
        {
            // checking same button state and button 
            if ( (current_state == 0) && (last_state == 1) )
            {
                display_clear_msg_text();
                processState = 0;   // switching state back to idle
            }
        }
        
        last_state = current_state;     // for button state

        // yield to scheduler for a bit between checks
        //printf("Minimum stack space left is: %u\r\n", uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}
