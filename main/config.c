#include "freertos/FreeRTOS.h"

#include "config.h"
#include "configCmd.h"
//#include "led.h"
#include "token.h"
#include "stdio.h"
#include "console.h"

#include "nvs_flash.h"

#define STORAGE_NAMESPACE "ESP32_CONFIG"

#define START_CONFIG 0xDEADBEAF
#define END_CONFIG 0xBEAFDEAD

//portMUX_TYPE osalSysMutex = portMUX_INITIALIZER_UNLOCKED;

config_t config = {
		.start_config = START_CONFIG,
		.hw_ver = BOARD_VERSION,
		.sw_ver = SOFTWARE_VERSION,
		.dev_type = DEFAULT_DEV_TYPE,
		.end_config = END_CONFIG};
uint8_t test_config_save(void);

int verify_config(config_t * conf)
{
	if (conf->start_config == START_CONFIG && conf->end_config == END_CONFIG) {
		if(memcmp(&config.hw_ver, &conf->hw_ver, sizeof(config.hw_ver)) != 0 
		|| memcmp(&config.sw_ver, &conf->sw_ver, sizeof(config.sw_ver)) != 0
		|| memcmp(&config.name, &conf->name, sizeof(config.name)) != 0) {
			config.can_id = conf->can_id;
			config.dev_type = conf->dev_type;
			return 0;
		}
		return 1;
	}
		
	return 0;
}

static void configInitStruct(config_t *con)
{
	if (con->can_id == 0)
		con->can_id = 0;
	memcpy(con->name, BOARD_NAME, sizeof(BOARD_NAME));
	configSave(con);
}

void configInit(void)
{
	// Read and verify config
	esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

	config_t read_config;
	#if CONFIG_USE_CONSOLE
	cosole_echo_flag = CONFIG_CONSOLE_ECHO;
	#endif
	if (configRead(&read_config) == ESP_OK) 
	{
		if (verify_config(&read_config) == TRUE) {
			memcpy(&config, &read_config, sizeof(config_t));
		}
		else {
			configInitStruct(&config);
		}
		if (config.can_id <= 0 || config.can_id >= 0x40)
			config.can_id = 0;
	}
	else
	{
		configInitStruct(&config);
	}
}

int configSave(config_t *config)
{
	nvs_handle my_handle;
    esp_err_t err;
	 // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
		nvs_close(my_handle);
		return err;
	}

	err = nvs_set_blob(my_handle, "config", config, sizeof(config_t));

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

esp_err_t configRead(config_t *config)
{
	nvs_handle my_handle;
    esp_err_t err;
	 // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;
	 // Read the size of memory space required for blob
    size_t required_size = 0;  // value will default to 0, if not set yet in NVS
    err = nvs_get_blob(my_handle, "config", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) 
	{
		nvs_close(my_handle);
		return err;
	}

    // Read previously saved blob if available
    if (required_size == sizeof(config_t)) {
        err = nvs_get_blob(my_handle, "config", config, &required_size);
		nvs_close(my_handle);
        return err;
    }
	nvs_close(my_handle);
	return ESP_ERR_NVS_NOT_FOUND;
}


void configRebootToBlt(void)
{
    esp_restart();
}

#if CONFIG_USE_CONSOLE

int configEcho(console_t *con, t_tokenline_parsed *p)
{
	(void) con;
	switch (p->tokens[1])
	{
		case T_ON:
			cosole_echo_flag = 1;
		break;

		case T_OFF:
			cosole_echo_flag = 0;
		break;

		default:
			return FALSE;
		break;
	}
	return TRUE;
}

int configMQTT(console_t *con, t_tokenline_parsed *p)
{
	#if CONFIG_USE_MQTT
	(void) con;
	switch (p->tokens[1])
	{
		case T_ADDRESS:
			strcpy(MQTTServerAddress, p->buf);
			MQTTDrvDisconnect();
		break;
		case T_PORT:
			memcpy(&MQTTServerPort, p->buf, sizeof(uint32_t));
			MQTTDrvDisconnect();
		break;
		case T_LIST:
			consolePrintfTimeout(con, 100, MQTTServerAddress);
			consolePrintfTimeout(con, 100, ":%d\n\r", MQTTServerPort);
		break;
		default:
			return FALSE;
		break;
	}
	#endif
	return TRUE;
}

int configReset(console_t *con, t_tokenline_parsed *p)
{
	(void) con;
	(void) p;
	configRebootToBlt();
	return TRUE;
}

void iterate(console_t *con, config_t *aStruct)
{
	consolePrintfTimeout(con, 100, "can_id: 0x%X\n\r", aStruct->can_id);
	consolePrintfTimeout(con, 100, "name: %s\n\r", BOARD_NAME);
	consolePrintfTimeout(con, 100, "hw_ver: %u.%u.%u\n\r", aStruct->hw_ver[0], aStruct->hw_ver[1], aStruct->hw_ver[2]);
	consolePrintfTimeout(con, 100, "sw_ver: %u.%u.%u\n\r", aStruct->sw_ver[0], aStruct->sw_ver[1], aStruct->sw_ver[2]);
	consolePrintfTimeout(con, 100, "dev type: ");
	switch(config.dev_type)
	{
		case T_DEV_TYPE_SERVER:
		consolePrintfTimeout(con, 100, "server\n\r");
		break;
		case T_DEV_TYPE_CLIENT:
		consolePrintfTimeout(con, 100, "client\n\r");
		break;
		default:
		consolePrintfTimeout(con, 100, "None\n\r");
		break;
	}
}

int configCmd(console_t *con, t_tokenline_parsed *p)
{
	(void) con;
	int err = 1;

	switch (p->tokens[1])
	{
	case T_LIST:
		iterate(con, &config);
		break;
	case T_DEV_TYPE:
		config.dev_type = p->tokens[2];
		break;
	case T_SAVE:
		err = configSave(&config);
		break;
	case T_BOOT:
		configRebootToBlt();
		break;	
	case T_CAN_ID:
		memcpy(&config.can_id, p->buf, sizeof(uint32_t));
		break;

	default:
		err = -1;
		break;
	}

	if (err)
	{
		consolePrintf("ERR: %d\r\n\r", err);
		return FALSE;
	}
	else
	{
		consolePrintf("OK\r\n\r", err);
		return TRUE;		
	}
}
#endif