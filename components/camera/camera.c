#define ESP32 (0)

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

#include "GUI_drivers.h"
#include "wifi_comms.h"


// ==== Defines For Camera ================================

#define CAM_BUTTON   38

// defines for esp32s3 specifically
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    15
#define SIOD_GPIO_NUM    4
#define SIOC_GPIO_NUM    5
#define VSYNC_GPIO_NUM   6
#define HREF_GPIO_NUM    7
#define PCLK_GPIO_NUM    13

#define Y9_GPIO_NUM      16
#define Y8_GPIO_NUM      17
#define Y7_GPIO_NUM      18
#define Y6_GPIO_NUM      12
#define Y5_GPIO_NUM      10
#define Y4_GPIO_NUM      8
#define Y3_GPIO_NUM      9
#define Y2_GPIO_NUM      11




// ==== Defined Variables and Structures =================

// static frame buffer for us work with and reuse
static camera_fb_t *pic;

// pre-defined settings for our camera
static camera_config_t camera_config = {

    .ledc_channel = LEDC_CHANNEL_0,
    .ledc_timer = LEDC_TIMER_0,

    .pin_d0 = Y2_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d7 = Y9_GPIO_NUM,

    .pin_xclk = XCLK_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,

    // Use 10MHz XCLK for better performance
    .xclk_freq_hz = 2 * 1000 * 1000,

    // Always use JPEG for performance & memory efficiency
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_VGA,   // 800x600 resolution

    // JPEG settings
    .jpeg_quality = 10, // Lower value = better quality
    .fb_count = 2,      // Double buffering for continuous capture

    // Store frame buffer in PSRAM
    //.fb_location = CAMERA_FB_IN_PSRAM,
    .fb_location = CAMERA_FB_IN_DRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,

#if ESP32
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

#endif

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

    // TODO: see if this is required or not - was causing issue with camera load
    // changing some of the sensor settings for better quality
    sensor_t *s = esp_camera_sensor_get();
    s->set_gain_ctrl(s, 0); // auto gain off (1 or 0)
    s->set_exposure_ctrl(s, 0); // auto exposure off (1 or 0)
    s->set_agc_gain(s, 0); // set gain manually (0 - 30)
    s->set_aec_value(s, 600); // set exposure manually (0-1200)
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



// Task to be used for polling the wifi button press and starting camera operations
void camera_button_poll(void* params)
{
    int lastState = 1;

    gpio_config_t io_config = {
        .pin_bit_mask = (1ULL << CAM_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_config);


    for (;;)
    {
        int currentState = gpio_get_level(CAM_BUTTON);

        if ( currentState == 0 && lastState == 1 )
        {
            // prmpt user for image capture
            write_to_disp_temp("Capturing photo...", 1);

            // taking a picture
            if (take_picture() == ESP_OK) {

                camera_fb_t *pic = get_fb();    // getting frame buffer after successful image capture
                
                // attempting to send image to server for response
                esp_err_t state = send_image_to_server(pic);
                if (state == ESP_OK) {
                    printf("HTTP transmission success!\n");
                }
                else {
                    printf("something went wrong during HTTP transmission\n");
                }

            }
            else {
                write_to_disp_temp("Issue occured whilst capturing image...", 3);
            }

        }
        lastState = currentState;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

}


