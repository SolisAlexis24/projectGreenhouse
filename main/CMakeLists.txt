# El siguiente conjunto de líneas recupera el nombre del componente.
set(COMPONENT_REQUIRES "")
set(COMPONENT_PRIV_REQUIRES 
            esp_driver_i2c 
            esp_driver_gpio
            nvs_flash
            LCD1602 
            AM2302
            ADC
            WiFi
            PWM
            zeroCross
            PIDControl)

idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES ${COMPONENT_REQUIRES}
                    PRIV_REQUIRES ${COMPONENT_PRIV_REQUIRES})