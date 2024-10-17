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
#include "camera.h"
#include "esp_camera.h"

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"




// ==== testing ======================




#define MOUNT_POINT "/sdcard"

void save_picture(const char *filename, uint8_t *image_data, size_t image_size)
{
    FILE *f = fopen(filename, "w");
    if (f == NULL)
    {
        printf("failed to open card for writing\n");
        return;
    }

    fwrite(image_data, 1, image_size, f);
    fclose(f);

    printf("SD CARD - File saved: %s\n", filename);
}



void app_main(void)
{
    printf("gello");

    //check if camera init'd properly
    if ( init_camera() == ESP_OK ) {
        printf("main function init'd camera\n");
    }   

    //attempt to take picture
    if ( take_picture() == ESP_OK )
    {   
        //get picture from camera.h after taking it
        camera_fb_t *pic = get_fb();
        if (pic != NULL) {
            // error checking
            printf("main function took and received the picture\n");
        }
    }


    // ==== SD CARD SHIT ============================================================

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;

    esp_vfs_fat_sdmmc_mount_config_t mount_config_t = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config_t, &card);
    if (ret != ESP_OK)
    {
        printf("failed to mount the file system :( \n)");
        return;
    }
    
    sdmmc_card_print_info(stdout, card);
    printf("SD card mounted successfully");

    // ==============================================================================


}
