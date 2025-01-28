idf_component_register(SRCS "rf_comms.cpp"
                        INCLUDE_DIRS "."
                        REQUIRES driver RadioLib esp_timer EspHal GUI_drivers
                    )
