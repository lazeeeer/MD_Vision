#include "esp_camera.h"

esp_err_t init_camera();
camera_fb_t* get_fb();
esp_err_t take_picture();