/*
    - This folder contains all the code needed to work with the SPI bus
    - Commands include initializing the bus, sending cmds, sending data, etc...
*/

#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"


// ==== Defines needed for SPI as well as other handles ===============
#define RF_HOST    SPI2_HOST

#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22
#define PIN_NUM_DC   21
#define PIN_NUM_RST  18
#define PIN_NUM_BCKL 5

spi_device_handle_t spi;


// ==== Struct used for organizing message data =======================
typedef struct {

    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes;

} panel_init_cmd_t;


// ==== Code used to send a cmd byte or data byte(s) ==================
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
        printf("No data was passed to the functino; disp_send_data()\n");
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


void init_spi()
{
    //Initialize GPIO pins that will be used for SPI purposes
    gpio_config_t io_conf = {};
    // making one big bit mask of the GPIOs we want and setting them to pulled-up and output pins
    io_conf.pin_bit_mask = ((1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST) | (1ULL << PIN_NUM_DC));
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = true;
    gpio_config(&io_conf);  //writing these settings to the GPIO pins all together

    // Initialize the SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .max_transfer_sz = 1,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_MASTER_FREQ_26M, // Clock speed
        .mode = 0,                             // SPI mode
        .spics_io_num = PIN_NUM_CS,                // CS pin
        .queue_size = 1,                       // Queue size
    };

    //Initialize the SPI bus
    esp_err_t ret = spi_bus_initialize(RF_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    //Attach the LCD to the SPI bus
    ret = spi_bus_add_device(RF_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
}