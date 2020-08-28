#include "menu_param.h"
#include "parse_cmd.h"
#include "menu.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "console.h"

#define STORAGE_NAMESPACE "MENU"

#define debug_msg(...) //consolePrintfTimeout(&con0serial, CONFIG_CONSOLE_TIMEOUT, __VA_ARGS__)

static menuPStruct_t menuParameters[] = 
{
	[MENU_MOTOR] = {.max_value = 100, .default_value = 0},
	[MENU_SERVO] = {.max_value = 100, .default_value = 0},
	[MENU_MOTOR_IS_ON] = {.max_value = 1, .default_value = 0},
	[MENU_SERVO_IS_ON] = {.max_value = 1, .default_value = 0},
	[MENU_MOTOR_ERROR_IS_ON] = {.max_value = 1, .default_value = 0},
	[MENU_SERVO_ERROR_IS_ON] = {.max_value = 1, .default_value = 0},
	[MENU_CURRENT_SERVO] = {.max_value = 0xFFFF, .default_value = 0},
	[MENU_CURRENT_MOTOTR] = {.max_value = 0xFFFF, .default_value = 0},
	[MENU_VOLTAGE_ACCUM] = {.max_value = 0xFFFF, .default_value = 0},
	[MENU_ERRORS] = {.max_value = 0xFFFF, .default_value = 0},

	[MENU_ERROR_SERVO] = {.max_value = 1, .default_value = 1},
	[MENU_ERROR_MOTOR] = {.max_value = 1, .default_value = 1},
	[MENU_ERROR_SERVO_CALIBRATION] = {.max_value = 99, .default_value = 20},
	[MENU_ERROR_MOTOR_CALIBRATION] = {.max_value = 99, .default_value = 50},
	[MENU_BUZZER] = {.max_value = 1, .default_value = 1},
	[MENU_CLOSE_SERVO_REGULATION] = {.max_value = 99, .default_value = 50},
	[MENU_OPEN_SERVO_REGULATION] = {.max_value = 99, .default_value = 50},
	[MENU_TRY_OPEN_CALIBRATION] = {.max_value = 10, .default_value = 8},
};


static uint32_t menuSaveParameters_data[sizeof(menuParameters)/sizeof(menuPStruct_t)];
#define PARAMETERS_TAB_SIZE sizeof(menuParameters)/sizeof(menuPStruct_t)

void menuPrintParameters(void)
{
	for (uint8_t i = 0; i < PARAMETERS_TAB_SIZE; i++) {
		debug_msg("%d : %d\n", i,  menuSaveParameters_data[i]);
	}
}

esp_err_t menuSaveParameters(void) {
	
	nvs_handle my_handle;
    esp_err_t err;
	 // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
		nvs_close(my_handle);
		return err;
	}

	err = nvs_set_blob(my_handle, "menu", menuSaveParameters_data, sizeof(menuSaveParameters_data));

	if (err != ESP_OK) {
		nvs_close(my_handle);
		return err;
	}

    // Commit
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
		nvs_close(my_handle);
		return err;
	}
    // Close
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t menuReadParameters(void) {
	nvs_handle my_handle;
    esp_err_t err;
	 // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;
	 // Read the size of memory space required for blob
    size_t required_size = 0;  // value will default to 0, if not set yet in NVS
    err = nvs_get_blob(my_handle, "menu", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
		nvs_close(my_handle);
		return err;
	}

    // Read previously saved blob if available
    if (required_size == sizeof(menuSaveParameters_data)) {
        err = nvs_get_blob(my_handle, "menu", menuSaveParameters_data, &required_size);
		nvs_close(my_handle);
        return err;
    }
	nvs_close(my_handle);
	return ESP_ERR_NVS_NOT_FOUND;
}

void menuSetDefaultValue(void) {
	for(uint8_t i = 0; i < sizeof(menuSaveParameters_data)/sizeof(uint32_t); i++)
	{
		menuSaveParameters_data[i] = menuParameters[i].default_value;
	}
}

uint32_t menuGetValue(menuValue_t val) {
	if (val >= MENU_LAST_VALUE)
		return 0;
	return menuSaveParameters_data[val];
}

uint32_t menuGetMaxValue(menuValue_t val) {
	if (val >= MENU_LAST_VALUE)
		return 0;
	return menuParameters[val].max_value;
}

uint8_t menuSetValue(menuValue_t val, uint32_t value) {
	if (val >= MENU_LAST_VALUE)
		return FALSE;

	if (value > menuParameters[val].max_value) {
		return FALSE;
	}

	menuSaveParameters_data[val] = value;
	//ToDo send to Drv
	return TRUE;
}

void menuParamInit(void) {
	if (menuReadParameters() != ESP_OK) {
		menuSetDefaultValue();
	}
}