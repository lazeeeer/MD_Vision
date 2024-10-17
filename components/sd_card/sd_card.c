# include <stdio.h>
#include "sd_card.h"

// sdmmc peripheral libraries and fat file system
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

// esp-camera library in case needed
#include "esp_camera.h"


// ==== Defined Variables and Structures =================================

// static card object to work with our SD card
static sdmmc_card_t* card;


// ==== Function Calls ==================================================

// function to init and mount the SD card for use
void init_sd_card()
{
    // defining the host peripheral for the SD card
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;

    // defining parameters for the slot
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;

    // information regarding mounting WITH the fat file system
    esp_vfs_fat_sdmmc_mount_config_t mount_config_t = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    // mount the Sd card and tech to see if it mounted properly
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config_t, &card);
    if (ret != ESP_OK)
    {
        printf("init_sd_card(): Failed to mount the file system\n)");
        return;
    }

    // print out info to serial monitor for debugging
    sdmmc_card_print_info(stdout, card);
}


// function to save a picture to the SD card
void save_picture(const char *filename, uint8_t *image_data, size_t image_size)
{
    // open a file on the SD card to write with
    FILE *f = fopen(filename, "w");
    
    if (f == NULL)  // checking to see if we could open the file
    {   
        printf("save_picture(): failed to open card for writing\n");
        return;
    }

    // write the frame buffer (or any date) to the SD card
    fwrite(image_data, 1, image_size, f);
    fclose(f);

    // debugging output
    printf("SD CARD - File saved: %s\n", filename);
}

