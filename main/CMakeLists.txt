idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES config backend drv menu spi_flash pthread)