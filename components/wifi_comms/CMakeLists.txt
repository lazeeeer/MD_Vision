idf_component_register(SRCS "wifi_comms.c"
                        INCLUDE_DIRS "."
                        REQUIRES driver esp_http_client esp_wifi esp_event nvs_flash
                    )
