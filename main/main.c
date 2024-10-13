#include <stdio.h>
#include <string.h>

// custom code and wrappers
#include "GUI_drivers.h"
#include "SPI_drivers.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "u8g2.h"
#include "u8g2_esp32_hal.h"


void app_main(void)
{
    printf("gello");
    init_display();

    clear_disp();
    write_to_disp(0, 10, "sleep time");
}
