#ifndef SYNC_OBJECTS_H
#define SYNC_OBJECTS_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"




// --- declaring all the globals needed for control below... --- //
extern SemaphoreHandle_t xMsgBufferSemphr;

//size_t MAX_MESSAGE_LEN = 128;

#endif // SYNC_OBJECTS_H

