
esp_err_t init_wifi_comms();
//void test_http_request();
esp_err_t send_image_to_server( camera_fb_t *fb );
esp_err_t http_ping_server(const char* url);
void parse_json(const char *jsonString);