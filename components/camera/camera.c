
// generic c includes
#include <stdio.h>
#include <string.h>

// gpio and driver includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

//esp camera libraries
#include "camera.h"
#include "esp_camera.h"


// ==== Defines For Camera ================================

// specifically for esp32-wrover
#define CAM_PIN_PWDN -1  //power down is not used
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 21
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27
#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 19
#define CAM_PIN_D2 18
#define CAM_PIN_D1 5
#define CAM_PIN_D0 4
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22


// ==== Defined Variables and Structures =================

// static frame buffer for us work with and reuse
static camera_fb_t *pic;

// pre-defined settings for our camera
static camera_config_t camera_config = {

    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,
    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    // chose 10 MHz - seems to work better
    .xclk_freq_hz = 10000000,
    // for camera blink on picture
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    // use jpeg for ease of use after exporting out
    // SVGA = 600x800 frame size, should suffice for resolution
    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_SVGA,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    // quality of 12 shuold suffice
    // fb = 2 for double buffering if needed
    // ensure PSRAM is enabled for storing
    .jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 2,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

// ==== Function calls ===================================

// init the camera with our pre-defined settings above
esp_err_t init_camera()
{   
    // initing camera hardware with our config
    esp_err_t err = esp_camera_init(&camera_config);

    // quick error check
    if (err != ESP_OK) {
        printf("init_camera(): Could not init camera properly\n");       // REPLACE THIS WITH ESP LOGGING EVENTAULLY
        return ESP_FAIL;
    }
    else {
        return ESP_OK;
    }
}


// return the contents of the current frame buffer
camera_fb_t* get_fb()
{
    return pic;
}


// take a picture and return the frame buffer containing it
esp_err_t take_picture()
{
    // making sure the buffer is free before taking another picture
    if (pic != NULL) {
        esp_camera_fb_return(pic);
    }
    // debug prompt
    printf("get_picture(): taking picture now\n");

    // attemping to capture a photo in the fb
    pic = esp_camera_fb_get();

    // checking if photo was captured correctly
    if (pic == NULL)
    {
        printf("get_picture(): Capture failed!\n");
        return ESP_FAIL;
    }
    printf("picture taken!, lenght of %zu bytes!\n", pic->len);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    return ESP_OK;
}
