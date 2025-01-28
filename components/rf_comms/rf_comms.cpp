
#include <iostream>
#include <string>
#include <RadioLib.h>

#include "rf_comms.h"
#include "EspHal.h"
#include "esp_err.h"


#ifdef __cplusplus
extern "C" {
#endif

    #include "GUI_drivers.h"

#ifdef __cplusplus
}
#endif

using namespace std;

// Defines of pins needed for RadioLib SPI interface
#define SPI_MOSI_PIN 23
#define SPI_MISO_PIN 19
#define SPI_SCK_PIN  18
#define SPI_CS_PIN    5  // this is for RF only
#define RFM_RESET_PIN 4  // this is for RF only


// ==== Static items for controlling display ========== //
static EspHal* hal = new EspHal(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);
static RF69 radio = new Module(hal, SPI_CS_PIN, 26, RFM_RESET_PIN, 15);
static PagerClient pager(&radio);


// ==== Functions for init'ing and interacting with module ========== //

//TODO: TEST ALL THE PAGER CODE NOW AS CALLABLE FUNCTIONS

extern "C" int testFunc(void)
{
    return 420;
}

// init the module and put it into a pager mode that will LISTEN only
esp_err_t init_radio(void)
{
    int state;   // variable for checking state of the RadioLib calls
    const int digitalDataIn = 32;
    uint32_t myAddress = 12345;

    Module *myModule = radio.getMod();

    // turning on radio
    state = radio.begin();
    if (state == RADIOLIB_ERR_NONE)
    {
        printf("radio started just fine!\n");
    } else {
        printf("Failed on radio.begin()\n");
        return ESP_FAIL;
    }


    // starting the radio as a PAGER
    state = pager.begin(434.0, 1200, false, 4500);
    if (state == RADIOLIB_ERR_NONE)
    {
        printf("radio started just fine!\n");
    } else {
        printf("Failed on pager.begin()\n");
        return ESP_FAIL;
    }

    // putting the pager into RECEIVE mode
    state = pager.startReceive(digitalDataIn, myAddress);
    if (state == RADIOLIB_ERR_NONE)
    {
        printf("radio started just fine!\n");
    } else {
        printf("Failed on startReceive()\n");
        return ESP_FAIL;
    }

    // pager is ready to be read from using the readData() function when
    // a message is seen using the .available() function
    return ESP_OK;  // made it to end wihtout failure
}


// check and return number of available messages on the pager
int get_numMessages()
{
    size_t messages = pager.available();

    if ( messages > 0 ) {
        return messages;
    }
    else {
        return 0;
    }
}


// function to read a message and return len of packet
int get_message(uint8_t* byteBuffer, size_t bufferLen )
{
    size_t len = bufferLen;                 // len of packet received -> for error checking
    uint32_t rec_address;       // address that sent the packet

    // filling buffer from calling function using readData()
    int state = pager.readData(byteBuffer, &len, &rec_address);

    if (state != RADIOLIB_ERR_NONE)
    {
        printf("could not read a message for some reason... : %d\n", state);
        return state;   // returning RadioLib error code for debug
    }

    printf("Received %d bytes from address %lu\n", len, rec_address);

    if ( len == 0 || len > bufferLen )
    {
        printf("either no data received or len > bufferLen...\n");
        return -1;
    }

    return RADIOLIB_ERR_NONE;
}


// main task look that checks if there is a tranmission available
void receive_transmission(void *param)
{
    uint8_t buffer[256];
    size_t length = sizeof(buffer);
    // int state;
    // const int digitalDataIn = 12;
    // uint32_t myAddress = 12345;
    int num;

    for (;;)
    {       
        num = get_numMessages();

        // checking if a message is available
        if ( num > 0 )
        {
            printf("MESSAGE AVAILABLE:%d\n", num);

            // trying to read a message:
            if ( get_message(buffer, length) == 0) 
            {   
                //printf("%s\n", buffer);   // debug printing


                // resetting the buffer 
                memset(buffer, 0, sizeof(buffer));
            }
            else {
                printf("something failed in the else?\n");
            }
        }
        else {
            printf("MESSAGE AVAILABLE:%d\n", num);
            printf("no message received yet...\n");
        }
        vTaskDelay(pdMS_TO_TICKS(500));

    }
}

