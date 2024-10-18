#include <stdio.h>
#include <string.h>

// custom code and wrappers
#include "GUI_drivers.h"
#include "SPI_drivers.h"

// including FreeRTOS driver libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

// including graphics libraries
#include "u8g2.h"
#include "u8g2_esp32_hal.h"

// including camera and sd libraries
#include "esp_camera.h"
#include "camera.h"
#include "sd_card.h"

// including UART libraries
#include "driver/uart.h"



// ==== testing ======================


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

    // loop that constantly runs and checks for shit
    for (;;)
    {
        
        // reading 1 single byte of data
        len = uart_read_bytes(UART_PORT_NUM, &data, 1, 0);

        // checking to see if there is data on the buffer
        if (len > 0)
        {
            if (data == '\n' || data == '\r')
            {
                // adding a null temrinator
                stringBuffer[position] = '\0';

                if(xQueueSend(q, stringBuffer, portMAX_DELAY) != pdPASS)
                {
                    printf("could not put msg in queue :(\n)");
                }

                memset(stringBuffer, 0, strlen(stringBuffer));
                uart_flush(UART_PORT_NUM);
                position = 0;

            }
            else {
                stringBuffer[position++] = data;
            }

            uart_flush(UART_PORT_NUM);
        }

        //adding to control can go back to scheduler...
        vTaskDelay(500/portTICK_PERIOD_MS);
    }
}


// task to taken in data and update the display
void queue_to_disp(void *param)
{

    char msg[MSG_CHAR_LEN];

    for (;;)
    {
       if (xQueueReceive(q, msg, portMAX_DELAY) == pdPASS)
       {
            printf("queue has message\n");
            printf("received: %s\n", msg);
            memset(msg, 0, MSG_CHAR_LEN);
        }

        // giving control back to scheduler
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
    // end of task...
}





void app_main(void)
{

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

    //msg = (char*)malloc(sizeof(char) * MSG_CHAR_LEN);

    // initing the display and making sure its clear
    init_display();
    clear_disp();


    // CREATING TASKS

    xTaskCreate(UART_input, "reading the UART", 2048, NULL, 1, NULL);
    xTaskCreate(queue_to_disp, "passing info read to disp", 2024, NULL, 1, NULL);



    // end of main...
}
