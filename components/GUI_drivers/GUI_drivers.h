#include <stdio.h>

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

    display_msg_type_t msg_flag;    // flag describing type of message
    char message[64];               // raw message

}display_msg_package_t;




void init_display();
void clear_disp();
void write_to_disp(int x, int y, const char* str);
void test_pixels();

