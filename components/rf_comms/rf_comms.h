// testing calling cpp fucntion from c files

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

    int testFunc(void);

    // making these functions callable from .c files
    esp_err_t init_radio(void);
    int get_numMessages();
    int get_message( uint8_t* byteBuffer, size_t bufferLen );

    void receive_transmission(void *param);     // debug task
    void poll_radio(void *param);               // main msg poll task

#ifdef __cplusplus
}
#endif



