idf_component_register(SRCS "zeroCross.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_gpio
                    REQUIRES esp_driver_gptimer)