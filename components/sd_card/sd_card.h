#include <stdio.h>
#include "esp_camera.h"

void save_picture(const char *filename, uint8_t *image_data, size_t image_size);
void init_sd_card();
