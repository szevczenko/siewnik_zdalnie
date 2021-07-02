idf_component_register(SRCS "menu_param.c" "menu.c" "ssd1306_fonts.c" "ssd1306_tests.c" "ssd1306.c" "ssdFigure.c"
                    INCLUDE_DIRS "." 
                    REQUIRES config backend menu main nvs_flash)
