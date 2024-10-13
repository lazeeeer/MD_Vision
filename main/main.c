#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "test.h"   // adding in my custom component

// Define your display pins
#define PIN_SCK  18 // SPI clock
#define PIN_MOSI 23 // SPI MOSI
#define PIN_CS   5  // Chip select
#define PIN_DC   0 // Data/Command
#define PIN_RST  2 // Reset

// GLOBAL VARIABLE HOLDING DISPLAY SPI OBJECT
spi_device_handle_t spi_display_handle;


// INIT SPI BUS CODE
void init_spi_bus()
{

    gpio_set_direction(PIN_CS, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_RST, GPIO_MODE_OUTPUT);

    // Reset the display
    gpio_set_level(PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10)); // Delay for reset
    gpio_set_level(PIN_RST, 1);

    // Initialize the SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = PIN_MOSI,
        .sclk_io_num = PIN_SCK,
        .max_transfer_sz = 0
    };

    spi_bus_free(VSPI_HOST);
    //calling it fucntion to apply all previous configs
    spi_bus_initialize(VSPI_HOST, &buscfg, 1);

    // --- ADDING THE DISPLAY TO THE SPI BUS -----------
    // Initialize the SPI bus (assuming spi_bus_initialize() is already called)
    spi_device_interface_config_t dev_config = {
        .clock_speed_hz = SPI_MASTER_FREQ_26M, // Clock speed
        .mode = 0,                             // SPI mode
        .spics_io_num = PIN_CS,               // CS pin
        .queue_size = 1,                       // Queue size
    };

    esp_err_t ret = spi_bus_add_device(SPI2_HOST, &dev_config, &spi_display_handle);
    if (ret != ESP_OK) {
        //Handle this later if it becomes an issue
        printf("ERROR OCCURED WHEN ADDING DISPLAU DEVICE");
    }
}


// FUNCTION MADE FOR SPECIFICALLY TALKING TO DISPLAY

void sendDispCmd(uint8_t cmd, uint8_t *data, size_t data_len)
{
    //getting transaction structure ready and init to 0
    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));

    // setting up the transaction structure for data sizes
    transaction.length = (data_len + 1) * 8;
    transaction.tx_buffer = malloc(transaction.length / 8);
    if (transaction.tx_buffer == NULL)      // check if tx_buffer wasnt able to be malloced
    {
        printf("COULD NOT ALLOCATE TO TRANSACTION BUFFER FOR SOME READON");
        return;
    }

    // filling the buffer with the data in the function parameters
    uint8_t *buffer = (uint8_t)transaction.tx_buffer;   //getting a pointer to the data array
    buffer[0] = cmd; // First byte being sent is the command
    if (data && data_len > 0) {     // if data is present memcpy is to the rest of the data buffer to send
        memcpy(&buffer[1], data, data_len);
    }

    // error check to see if anything went wrong when we try and transmit
    esp_err_t ret = spi_device_transmit(spi_display_handle, &transaction);
    if (ret != ESP_OK) {
        printf("FAILED TO TRANSMIT DATA: %s\n", esp_err_to_name(ret));
    }

    //free the transaction buffer when we are done to avoid leaks
    free(transaction.tx_buffer);

}




// --- SPI WRITE CMD AND WRITE DATA FUNCTIONS -------
void spi_write_command(uint8_t cmd)
{
    gpio_set_level(PIN_CS, 0);  //select display 
    gpio_set_level(PIN_DC, 0);  //command mode

    // setting up a trans package and memset'ing it to 0
    spi_transaction_t trans;
    memset(&trans, 0, sizeof(trans));
    trans.length = 8;

    //fill tx buffer witht the cmd and send it on the bus
    trans.tx_buffer = &cmd;
    spi_device_transmit(spi_display_handle, &trans);

    // error check to see if anything went wrong when we try and transmit
    esp_err_t ret = spi_device_transmit(spi_display_handle, &trans);
    if (ret != ESP_OK) {
        printf("FAILED TO TRANSMIT DATA: %s\n", esp_err_to_name(ret));
    }

    free(trans.tx_buffer);
    gpio_set_level(PIN_CS, 1);  //deselect display
}


void spi_write_data(const uint8_t *data, size_t len)
{
    gpio_set_level(PIN_CS, 0); //select display
    gpio_set_level(PIN_DC, 1); //data mode

    // setting up a trans package and memset'ing it to 0
    spi_transaction_t trans;
    memset(&trans, 0, sizeof(trans));
    trans.length = len * 8;

    //filling trans tx_buffer with my data
    trans.tx_buffer = data;
    spi_device_transmit(spi_display_handle, &trans);

    // error check to see if anything went wrong when we try and transmit
    esp_err_t ret = spi_device_transmit(spi_display_handle, &trans);
    if (ret != ESP_OK) {
        printf("FAILED TO TRANSMIT DATA: %s\n", esp_err_to_name(ret));
    }
    
    free(trans.tx_buffer);      //freeing data in the trans buffer
    gpio_set_level(PIN_CS, 1);  //deselect display
}


// Function to initialize the display
void my_display_init(void) {

    gpio_set_level(PIN_CS, 0); // Select the display

    // Example commands (specific to your display; check datasheet)
    // spi_write_command(0xAE); // Display off
    // spi_write_command(0xD5); // Set display clock
    // spi_write_command(0x80); // Set clock frequency
    // ... (additional initialization commands)

    gpio_set_level(PIN_CS, 1); // Deselect the display
}

/*
// Function to flush the display buffer
void my_display_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_map) {
    // Code to write `color_map` pixels to the display
    // Call disp_drv->flush_ready(disp_drv); when done

}
*/

void app_main(void)
{

    //calling my code to init the spi bus
    init_spi_bus();

    //command byte to TURN ON display
    uint8_t cmd = 0xAF;
    //uint8_t data[] = {0x00, 0xFF};

    spi_write_command(cmd);
    //spi_write_data(data, sizeof(data));



}
