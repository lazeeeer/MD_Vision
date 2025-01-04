#include <iostream>

// callable functions from rf_comms.c
void init_radio(void);
int get_numMessages();
int get_message( uint8_t* byteBuffer, size_t bufferLen );