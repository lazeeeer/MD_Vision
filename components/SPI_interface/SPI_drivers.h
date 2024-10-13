#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"

void disp_send_cmd(spi_device_handle_t spi, const uint8_t cmd, bool keep_cs_active);
void disp_send_data(spi_device_handle_t spi, const uint8_t *data, int len);
void init_spi();
