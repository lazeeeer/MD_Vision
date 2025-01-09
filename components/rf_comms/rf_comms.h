// testing calling cpp fucntion from c files

#ifdef __cplusplus
extern "C" {
#endif

    int testFunc(void);

    // making these functions callable from .c files
    void init_radio(void);
    int get_numMessages();
    int get_message( uint8_t* byteBuffer, size_t bufferLen );

#ifdef __cplusplus
}
#endif



