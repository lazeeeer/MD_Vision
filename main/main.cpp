// c++ includes

#include <iostream>
#include <RadioLib.h>

// custom cpp libraries
#include "rf_comms.h"

// extern c wrapper for original source code
#ifdef __cplusplus
extern "C" {
#endif

    #include <stdio.h>
    #include <string.h>

    // including graphics libraries
    #include "u8g2.h"
    #include "u8g2_esp32_hal.h"

    // including header files with all globals needed
    #include "sync_objects.h"

    // including component containing init and POST code


    // including camera and sd libraries
    #include "esp_camera.h"
    #include "camera.h"
    #include "sd_card.h"

    // custom code and wrappers
    #include "GUI_drivers.h"
    #include "SPI_drivers.h"

    // including FreeRTOS driver libraries
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/semphr.h"
    #include "driver/gpio.h"
    #include "driver/spi_master.h"

    // including UART libraries
    #include "driver/uart.h"

    // including wifi comms code
    #include "wifi_comms.h"

#ifdef __cplusplus
}
#endif




// ==== Definitions needed for main program ==== //

// global control objects...
SemaphoreHandle_t xMsgBufferSemphr = NULL;

// local variables for main...



// ==== Functions for main loop ================ //

esp_err_t init_globals(void)
{
    // init the msg buffer semaphore
    xMsgBufferSemphr = xSemaphoreCreateBinary();
    if (xMsgBufferSemphr == NULL)
    {
        printf("Message buffer semaphore could not be created...\n ");
        return ESP_FAIL;
    }

    return ESP_OK;
    // end of functino...
}




// ==== testing stuff ======================

#define UART_PORT_NUM UART_NUM_0
#define UART_BAUD_RATE 115200
#define UART_RX_BUF_SIZE 1024

QueueHandle_t q;

#define MSG_CHAR_LEN 128


// task to listen for input in UART buffer
void UART_input(void *param)
{
    size_t len = 0;
    uint8_t data;
    char stringBuffer[MSG_CHAR_LEN];     //buffer to take in data from UART and pass it on 
    int position = 0;

    for (;;)    // task loop...
    {   
        // entering loop to continuously read bytes if available
        while(1)
        {   
            //grabbing one byte of data
            len = uart_read_bytes(UART_PORT_NUM, &data, 1, portMAX_DELAY);

            // if there was a byte read, process it into the msg buffer
            if (len > 0)
            {   
                // if the byte was a termination char, process the msg and send it
                if (data == '\n' || data == '\r')
                {
                    // adding a null temrinator
                    stringBuffer[position] = '\0';

                    if(xQueueSend(q, stringBuffer, portMAX_DELAY) != pdPASS)
                    {
                        printf("could not put msg in queue :(\n)");
                    }

                    // clearing msg buffer and processing vars
                    memset(stringBuffer, 0, strlen(stringBuffer));
                    uart_flush(UART_PORT_NUM);
                    position = 0;

                }
                else    // regular processing, append last read byte to msg buffer
                {
                    stringBuffer[position++] = data;
                }
            }
            else { break; }     // if no bytes were read, break out of byte read loop
        }

        //adding to control can go back to scheduler...
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}


// task to taken in data and update the display
void queue_to_disp(void *param)
{
    char msg[MSG_CHAR_LEN];

    for (;;)
    {   
        // task yields until it gets a message on the queue
        if (xQueueReceive(q, msg, portMAX_DELAY) == pdPASS)
        {
            // debug printing...
            printf("queue has message\n");
            printf("received: %s\n", msg);

            //clear and write to disp
            //clear_disp();
            write_to_disp(msg);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            display_clear_msg_text();

            memset(msg, 0, MSG_CHAR_LEN);
        }
    }
    // end of task...
}



// KEEP THIS CODE - WORKING EXAMPLE OF GETTING RF COMMS WORKING
void receive_transmission(void *param)
{
    uint8_t buffer[64];
    size_t length = sizeof(buffer);
    int state;
    const int digitalDataIn = 12;
    uint32_t myAddress = 12345;

    // // turning on radio and setting parameters
    // state = radio.begin();
    // if (state == RADIOLIB_ERR_NONE) {
    //     printf("radio started just fine!\n");
    // }
    // state = pager.begin(434.0, 1200, false, 4500);
    // if (state == RADIOLIB_ERR_NONE)
    // {
    //     printf("radio started just fine!\n");
    // }

    // state = pager.startReceive(digitalDataIn, myAddress, 12345);

    // // error check to see if we turned on fine
    // if (state == RADIOLIB_ERR_NONE)
    // {
    //     printf("radio started just fine!\n");
    // }

    // task main loop...
    for (;;)
    {
        // checking the interrupt pin
        //printf("poll loop ran once...\n");

        // //size_t packets = pager.available();
        // if (packets > 0)
        // {
        //     std::cout << "there are this many packets: " << packets << std::endl;

        //     uint8_t byteBuff[128];
        //     size_t len = 0;
        //     uint32_t rec_addr;

        //     //state = pager.readData(byteBuff, &len, &rec_addr);
        //     printf("messages were received?? - message is: %s\n", byteBuff);
        // }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}




/*
    ***NOTE
    THIS ENTIRE FUNCTION IS JUST A SANDBOX RIGHT NOW FOR PROTOTYPING
    ADD/TEST ANYTHING YOU WANT IN HEAR AS A 'WRAPPER FUNCTION' CALLED FROM OTHER .C FILES
*/
extern "C" void app_main(void)
{
    // calling function to init all variables from the sync_objects.h file`
    esp_err_t initCheck = init_globals();
    if ( initCheck != ESP_OK )
    {
        printf("Something went wrong in initialization... Error: %s\n", esp_err_to_name(initCheck) );
        abort();
    }


    // ==== UART TESTING STUFF ======================== //

    // Configuration structure for UART parameters
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    // Install UART driver using an event queue here
    uart_driver_install(UART_PORT_NUM, UART_RX_BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //creating a queue to pass information around
    q = xQueueCreate(10, MSG_CHAR_LEN);

    // ==== UART TESTING STUFF ======================== //


    // init and test the display
    init_display();
    clear_disp();
    display_main_hud();


    // --- CREATING TASKS --- //
    xTaskCreate(UART_input, "reading the UART", 2048, NULL, 1, NULL);
    xTaskCreate(queue_to_disp, "passing info read to disp", 2024, NULL, 1, NULL);
    xTaskCreate(receive_transmission, "receive loop task", 4096, NULL, 1, NULL);
    //xTaskCreate(displayLoop, "displayLoop shid", 1024, NULL, 1, NULL);

    // end of main...
}
