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

    // library used for JSON parsing
    #include "cJSON.h"

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
    #include "esp_timer.h"

    // including UART libraries
    #include "driver/uart.h"

    // including wifi comms code
    #include "wifi_comms.h"

#ifdef __cplusplus
}
#endif

QueueHandle_t q;
#define MSG_CHAR_LEN 128


// ==== Macros for enabling / disabling certain parts of the code
#define ENABLE_WIFI (0)
#define ENABLE_UART (0)

#define ENABLE_STAT (0)


// ==== Definitions needed for main program ==== //
// global control objects...
SemaphoreHandle_t   xMsgDisplaySem = NULL;
QueueHandle_t       xMsgBufferQueue = NULL; 


// ==== UART definitions ===================== //
#define UART_PORT_NUM UART_NUM_0
#define UART_BAUD_RATE 115200
#define UART_RX_BUF_SIZE 1024

// Configuration structure for UART parameters
uart_config_t uart_config = {
    .baud_rate = UART_BAUD_RATE,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};


// ==== Code for tracking the CPU load of tasks ==== // 

void calculateTaskCpuLoad(TaskHandle_t taskHandle, uint32_t totalElapsedTime) {
    TaskStatus_t taskStatus;
    vTaskGetInfo(taskHandle, &taskStatus, pdTRUE, eRunning);

    uint32_t taskExecutionTime = taskStatus.ulRunTimeCounter;
    float cpuLoad = (float)taskExecutionTime / (float)totalElapsedTime * 100.0;
    printf("Task %s CPU Load: %.2f%%\n", taskStatus.pcTaskName, cpuLoad);
}

void printRunTimeStats() {
    char statsBuffer[512];
    vTaskGetRunTimeStats(statsBuffer);
    printf("Task Run-Time Stats:\n%s", statsBuffer);
}


// ==== Helper functions for the main loop ================ //
esp_err_t init_sync_objects(void)
{
    // init the msg buffer semaphore
    xMsgDisplaySem = xSemaphoreCreateBinary();
    if (xMsgDisplaySem == NULL)
    {
        printf("Message display semaphore could not be created...\n ");
        return ESP_FAIL;
    }
    xSemaphoreGive(xMsgDisplaySem);

    // init the msg buffer queue
    xMsgBufferQueue = xQueueCreate( 10, sizeof( char ) * 256 );
    if ( xMsgBufferQueue == NULL)
    {
        printf("Message buffer queue could not be created...\n ");
        return ESP_FAIL;
    }


    return ESP_OK;
}




// Main entry point of the program, init all objects in here and start tasks...
extern "C" void app_main(void)
{
    // needed to call for certain HALs
    gpio_install_isr_service(0);

    // call fucntion to init all the synchronization objects needed
    esp_err_t initCheck = init_sync_objects();
    if ( initCheck != ESP_OK )
    {
        printf("Something went wrong in initialization... Error: %s\n", esp_err_to_name(initCheck) );
        abort();
    }


    // ==== Initializing all the hardware needed for system ==================== //
    //init display
    // DISPLAY HAL WILL INIT SPI BUS FOR BOTH ITSELF AND RADIO MODULE
    if (init_display() == ESP_OK )
    {
        printf("Display has started!\n");
    }
    else {
        printf("Display couldnt start...\n");
        abort();
    }

    //init radio
    if (init_radio() == ESP_OK)
    {
        printf("Radio has started!\n");
    }
    else {
        printf("Radio couldnt start...\n");
    }

    // init camera
    if ( init_camera() == ESP_OK)
    {
        printf("Camera has started!\n");
    }
    else {

        printf("Camera couldnt start...\n");
        abort();
    }

    #if ENABLE_WIFI
        // init wifi
        if ( init_wifi_comms() == ESP_OK )
        {
            printf("Wifi has started!\n");
        }
        else {
            printf("Wifi couldnt start...\n");
            abort();
        }
    #endif


    // ==== TESTING SENDING IMAGE TO SERVER ==================== //
    // take_picture();
    // camera_fb_t *pic = get_fb();
    // printf("size of image is: %d\n", pic->len);

    // vTaskDelay(pdMS_TO_TICKS(1000));
    // esp_err_t state = send_image_to_server(pic);
    // if ( state != ESP_OK )
    // {
    //     printf("something went wrong\n");
    // }
    
    int64_t start_time = esp_timer_get_time();
        // generic function call...
    int64_t elapsted_time = esp_timer_get_time() - start_time;


    // Random ping testing code //
    // if ( http_ping_server("https://httpbin.org/get") == ESP_OK ) {
    //     printf("ping was succesful!\n");
    // }
    // else {
    //     printf("something wert wrong in ping...\n");
    // }


    // --- CREATING TASKS --- //
    xTaskCreate( poll_radio, "RadioTask", 4096, NULL, 5, NULL);
    xTaskCreate( displayLoop, "DisplayTask", 4096, NULL, 10, NULL);
    xTaskCreate( camera_button_poll, "CameraTask", 4096, NULL, 5, NULL);

    //xTaskCreate( receive_transmission, "receive loop task", 3072, NULL, 1, NULL);


    #if ENABLE_STAT
        for (;;)
        {
            // Calculate CPU load for each task
            uint32_t totalElapsedTime = (uint32_t)esp_timer_get_time();
            
            printf("\n");
            // calculateTaskCpuLoad(xTaskGetHandle("RadioTask"), totalElapsedTime);
            // calculateTaskCpuLoad(xTaskGetHandle("DisplayTask"), totalElapsedTime);
            // calculateTaskCpuLoad(xTaskGetHandle("CameraTask"), totalElapsedTime);
            printRunTimeStats();
            printf("\n");
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    #endif


    // end of main, begin Task running ...`
}