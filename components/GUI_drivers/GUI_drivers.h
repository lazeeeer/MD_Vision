#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Declare the queue handle as extern so other files can access it
extern QueueHandle_t displayQueue;


// Enum representing different message types....
// ie. hospital codes, basic message, urgent, etc...
typedef enum {

    // simple message types
    BASIC_MSG = 0,

    // code-based messages
    CODE_BLACK = 10,
    CODE_BLUE,
    CODE_RED,

    // task based messages
    ATTEND_ROOM = 20,
    PATIENT_CARE,

    INVALID = 99,

}display_msg_type_t;


typedef struct 
{
    char *f_name;
    char *l_name;
    char *last_checkup;

}display_msg_package_t;




esp_err_t init_display();
void clear_disp();
void write_to_disp(const char* str);
void test_pixels();
void display_clear_msg_text();

void display_main_hud(void);

void displayLoop(void *params);
void write_to_disp_temp(const char* str, int timeDly);
void write_patient_info(display_msg_package_t* patientInfo);


