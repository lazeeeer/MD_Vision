#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "test.h"   // adding in my custom component


// ---- Defines for the SPI comms bus --------------------------

// Define your display pins
#define LCD_HOST    SPI2_HOST

#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22

#define PIN_NUM_DC   21
#define PIN_NUM_RST  18
#define PIN_NUM_BCKL 5

// defining how many lines cane be sent in a single transfer on the SPI bus
// more lines means more memory usage
// make sure this is divisible by screen height
#define PARALLEL_LINES 16

// struct to define display INIT commands that need to be sent
typedef struct {

    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes;

} lcd_init_cmd_t;


// list of all the DMA commands that need to be send to the display
// forcing all commands into DRAM so when accessing with SPI we can use DMA 
DRAM_ATTR static const lcd_init_cmd_t SD1309_init_cmd_sequence[] = {

    {0xAE, {}, 0},          // Display OFF
    {0xD5, {0x80}, 1},      // Set Display Clock Divide Ratio / Oscillator Frequency
    {0xA8, {0x3F}, 1},      // Set Multiplex Ratio (adjust depending on your display size)
    {0xD3, {0x00}, 1},      // Set Display Offset
    {0x40, {}, 0},          // Set Display Start Line
    {0xA1, {}, 0},          // Set Segment Re-map (flips horizontally)
    {0xC8, {}, 0},          // Set COM Output Scan Direction (flips vertically)
    {0xDA, {0x12}, 1},      // Set COM Pins Hardware Configuration
    {0x81, {0x7F}, 1},      // Set Contrast Control
    {0xA4, {}, 0},          // Disable Entire Display ON
    {0xA6, {}, 0},          // Set Normal Display
    {0xD9, {0xF1}, 1},      // Set Pre-charge Period
    {0xDB, {0x20}, 1},      // Set VCOMH Deselect Level
    {0x8D, {0x14},  1},     // Enable Charge Pump
    {0xAF, {}, 0}           // Display ON
};


// ---- Function calls for the SPI bus --------------------------

/*
    Code to send a command BYTE on the SPI bus
    Takes in the SPI handle, a single byte command of data, and a bool as input
    passing true to the bool will keep the CS on for more tranmissions after
*/
void disp_send_cmd(spi_device_handle_t spi, const uint8_t cmd, bool keep_cs_active)
{
    // structures from spi class
    esp_err_t ret;
    spi_transaction_t transaction;

    memset(&transaction, 0, sizeof(transaction));   // init structure to be all zero
    transaction.length = 8;                         // we know one command is 1 byte long or 8 bits
    transaction.tx_buffer = &cmd;                   // the data is simply the command param we are passing
    transaction.user = (void*)0;                    // D/C needs to be set to zero to communicate a COMMAND to the disp driver

    // seeing if we want to keep CS on after sending a cmd
    if (keep_cs_active) {transaction.flags = SPI_TRANS_CS_KEEP_ACTIVE;}

    // sending the transaction structure and doing an assert to check if sending was alright
    ret = spi_device_transmit(spi, &transaction);
    assert(ret == ESP_OK);
}


/*
    Code to send a command BYTE on the SPI bus
    Takes in the SPI handle, an array of bytes for data, and a bool as input
    passing true to the bool will keep the CS on for more tranmissions after

    using spi_device_polling_transmit to send data and WAIT until the transfer is complete
    using polling mode since data is small so we can use the speed from polling mode
*/
void disp_send_data(spi_device_handle_t spi, const uint8_t *data, int len)
{
    // structures from spi class
    esp_err_t ret;
    spi_transaction_t transaction;

    //checking if data was even passed to the function
    if (len == 0) {
        printf("No data was passed to the functino; disp_send_data() ");
        return;
    }

    memset(&transaction, 0, sizeof(transaction));   // init structure to be all zero
    transaction.length = len * 8;                   // we know one command is 1 byte long or 8 bits
    transaction.tx_buffer = data;                   // the data is simply the command param we are passing
    transaction.user = (void*)1;                    // D/C needs to be set to 1 to communicate DATA to the disp driver

    //using the device polling command to transmit the data on the spi bus
    // checking the ret using an assert to see if tranmission was alright
    ret = spi_device_polling_transmit(spi, &transaction);
    assert(ret == ESP_OK);
}

/*
    Code to read the displays ID, use this as a sign of life when doing the init code
*/
uint32_t lcd_get_id(spi_device_handle_t spi)
{
    // When using SPI_TRANS_CS_KEEP_ACTIVE, bus must be locked/acquired
    spi_device_acquire_bus(spi, portMAX_DELAY);

    //get_id cmd
    // CHECK TO FIND WHAT OUR PANEL CMD IS
    lcd_cmd(spi, 0x04, true);

    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = 8 * 3;
    transaction.flags = SPI_TRANS_USE_RXDATA;
    transaction.user = (void*)1;

    esp_err_t ret = spi_device_polling_transmit(spi, &transaction);
    assert(ret == ESP_OK);

    // Release bus
    spi_device_release_bus(spi);

    return *(uint32_t*)transaction.rx_data;
}


// INIT SPI BUS CODE
void init_spi_bus()
{

    gpio_set_direction(PIN_NUM_CS, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);

    // Reset the display
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10)); // Delay for reset
    gpio_set_level(PIN_NUM_RST, 1);

    // Initialize the SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
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
        .spics_io_num = PIN_NUM_CS,                // CS pin
        .queue_size = 1,                       // Queue size
    };

    esp_err_t ret = spi_bus_add_device(SPI2_HOST, &dev_config, /* working on it */ );
    if (ret != ESP_OK) {
        //Handle this later if it becomes an issue
        printf("ERROR OCCURED WHEN ADDING DISPLAU DEVICE");
    }
}


void init_display(spi_device_handle_t spi)
{

    int cmd = 0;o


}






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
