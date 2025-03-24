
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
    #include "sync_objects.h"

#ifdef __cplusplus
}
#endif

using namespace std;

// Defines of pins needed for RadioLib SPI interface
#define SPI_MOSI_PIN    36 // was 35 // was 23
#define SPI_MISO_PIN    37 // was 36 // was 19
#define SPI_SCK_PIN     35 // was 37 // was 18
#define SPI_CS_PIN      21 // was 5  // this is for RF only
#define RFM_RESET_PIN   47 // was 4  // this is for RF only

#define DIO0_PIN        39   // was 26 
#define DIO1_PIN        40  // was 14  // might not be needed
#define DIO2_PIN        41   // was 13  // IMPORTANT -> should pass data from module to RadioLib

#define MSG_CHAR_LEN 256


// ==== Static items for controlling display ========== //
static EspHal2* hal = new EspHal2(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);
static RF69 radio = new Module(hal, SPI_CS_PIN, DIO0_PIN, RFM_RESET_PIN, DIO1_PIN);
static PagerClient pager(&radio);



// init the module and put it into a pager mode that will LISTEN only
esp_err_t init_radio(void)
{
    int state;   // variable for checking state of the RadioLib calls
    uint32_t myAddress = 12345;

    //Module *myModule = radio.getMod();

    // turning on radio
    state = radio.begin();
    if (state == RADIOLIB_ERR_NONE)
    {
        printf("radio started just fine!\n");
    } else {
        printf("Failed on radio.begin()\n");
        printf("state is: %d\n", state);
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
    state = pager.startReceive(DIO2_PIN, myAddress, 12345);
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
    else { return 0; }
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



// main task for polling the RF module for data and passing it to the msg queue
void poll_radio(void *param)
{
    uint8_t buffer[MSG_CHAR_LEN];
    size_t length = sizeof(buffer);

    for (;;)    // main task loop
    {  
        int num = get_numMessages();
        //printf("numer of messages is: %d\n", num);

        if (num > 0)   // message available in buffer
        {
            // getting data from the RadioLib software buffer
            if ( get_message(buffer, length) == RADIOLIB_ERR_NONE)
            {

                printf("message received: %s\n", buffer);   // debug prints:

                // add to the msg queue
                if ( xQueueSend(xMsgBufferQueue, buffer, portMAX_DELAY) != pdPASS )
                {
                    printf("Could not add msg to the queue for some reason...\n");
                }

                // clear buffer to ensure no leftover data
                memset(buffer, 0, sizeof(buffer));
            }
            else {
                printf("could not read message for some reason...\n");
            }
        }
        else {
            //printf("No message received yet...\n");
        }

        // this task will yeild for 1 second 
        // shouldnt matter how long due to hardware asynch behaviour

        //printf("Minimum stack sapce is: %u\r\n", uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}



// TESTING TASK TO DEBUG GETTING PAGER TRANMISSIONS
void receive_transmission(void *param)
{
    uint8_t buffer[MSG_CHAR_LEN];
    size_t length = sizeof(buffer);
    int num;
    // int state;
    // const int digitalDataIn = 12;
    // uint32_t myAddress = 12345;

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
                printf("Polled Msg: %s\n", buffer);   // debug printing
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

        printf("Task is using: %u\r\n", uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

