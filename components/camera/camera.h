#include "esp_camera.h"

esp_err_t init_camera();
camera_fb_t* get_fb();
esp_err_t take_picture();

// main task function
void camera_task( void *param );
void camera_button_poll(void* params);