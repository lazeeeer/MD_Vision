idf_component_register(SRCS "camera.c"
                        INCLUDE_DIRS "."
                        REQUIRES driver esp32-camera GUI_drivers wifi_comms
                    )
