# El siguiente conjunto de líneas recupera el nombre del componente.
set(COMPONENT_REQUIRES "")
set(COMPONENT_PRIV_REQUIRES esp_driver_i2c LCD1602)

idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES ${COMPONENT_REQUIRES}
                    PRIV_REQUIRES ${COMPONENT_PRIV_REQUIRES})