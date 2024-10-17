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


// ==== testing ======================

void app_main(void)
{
    printf("gello");

    //check if camera init'd properly
    if ( init_camera() == ESP_OK ) {
        printf("main function init'd camera\n");
    }   

    // init sd card
    init_sd_card();

    // test variable to hold image
    static camera_fb_t *pic;

    //attempt to take picture
    if ( take_picture() == ESP_OK )
    {   
        //get picture from camera.h after taking it
        pic = get_fb();
        if (pic != NULL) {
            // error checking
            printf("main function took and received the picture\n");
        }
    }

    // saving picture to SD card
    save_picture("/sdcard/myImage.jpg", pic->buf, pic->len);


    // end of main...
}
