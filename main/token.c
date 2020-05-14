#include "tokenline.h"
#include "token.h"
#include "cmd_utils.h"
#include "config.h"
#include "configCmd.h"
// #include "ethdrv.h"
#include "wifidrv.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#if CONFIG_USE_CONSOLE_TOKEN

tokenMap_t tokenMap[] =
	{
#if CONFIG_USE_CONSOLE_TOKEN_DEBUG
		{T_DEBUG, cmd_debug},
#endif //CONFIG_USE_CONSOLE_TOKEN_DEBUG
		{T_CLEAR, cmd_clear},
		{T_CONFIG, configCmd},
#if CONFIG_USE_SERIAL_PLOT
		{T_MONITOR, serialPlotMonitor},
#endif
		{T_ECHO, configEcho},
		{T_MQTT, configMQTT},
		{T_RESET, configReset},
		#if CONFIG_USE_WIFI
		{T_WIFI, configWiFi},
		#endif
		{0, NULL}};

// kolejność wpisów musi być zgodna z enum w token.h
t_token_dict tl_dict[] = {
	{0},
	{T_HELP, "help"},
	{T_CLEAR, "clear"},
#if CONFIG_USE_CONSOLE_TOKEN_DEBUG	
	{T_DEBUG, "debug"},
#endif //CONFIG_USE_CONSOLE_TOKEN_DEBUG	
	{T_CONFIG, "config"},

#if CONFIG_USE_SERIAL_PLOT
	{T_MONITOR, "monitor"},
#endif
	{T_ECHO, "echo"},
	{T_MQTT, "mqtt"},
	{T_ADDRESS, "address"},
	{T_PORT, "port"},

	{T_ON, "on"},
	{T_OFF, "off"},
	{T_LIST, "list"},
	{T_SET, "set"},
	{T_ADD, "add"},
	{T_RESET, "reset"},
	{T_SAVE, "save"},
	{T_BOOT, "boot"},
	{T_PERIOD, "period"},

	{T_CAN_ID, "can_id"},
	{T_DEV_TYPE, "dev_type"},

	{T_DEV_TYPE_SERVER, "server"},
	{T_DEV_TYPE_CLIENT, "client"},

#if CONFIG_USE_SERIAL_PLOT
	{T_TEST_CH_1, "test1"},
	{T_TEST_CH_2, "test2"},
	{T_TEST_CH_3, "test3"},
	{T_MONITOR_START_SEQ, "sequence"},
#endif
	
	{T_WIFI, "wifi"},
	{T_SSID, "ssid"},
	{T_SSID_NUMBER, "ssid_number"},
	{T_PASSWORD, "password"},
	{T_CONNECT, "connect"},
	{T_SHOW, "show"},
	{T_TOKENLINE, "tokenline"},
	{0}};

t_token tokens_wifi[] = {
	{T_SSID,
	.arg_type = T_ARG_STRING,
	.help = "SSID Access Point"},
	{T_PASSWORD,
	.arg_type = T_ARG_STRING,
	.help = "Password Access Point"},
	{T_SSID_NUMBER,
	.arg_type = T_ARG_UINT,
	.help = "Number Access Point"},
	{T_CONNECT,
	.help = "Connect"},
	{T_LIST,
	.help = "AP List"},
	{T_SHOW,
	.help = "Show parameters"},
	{0}
};

#if CONFIG_USE_CONSOLE_TOKEN_DEBUG
t_token tokens_debug[] = {
	{T_TOKENLINE,
	 .help = "Tokenline dump for every command"},
	{T_ON,
	 .help = "Enable"},
	{T_OFF,
	 .help = "Disable"},
	{0}};
#endif //CONFIG_USE_CONSOLE_TOKEN_DEBUG

t_token tokens_mqtt[] = {
	{T_ADDRESS,
	.arg_type = T_ARG_STRING,
	.help = "MQTT server address"},
	{T_PORT,
	.arg_type = T_ARG_UINT,
	.help = "MQTT server port"},
	{T_LIST,
	.help = "MQTT show parametrs"},
	{0}
};

t_token tokens_config_dev_type[] = {
	{T_DEV_TYPE_SERVER},
	{T_DEV_TYPE_CLIENT},
	{0}};

t_token tokens_config[] = {
	{T_CAN_ID,
	.arg_type = T_ARG_UINT,
	.help = "Can id"},
	{T_DEV_TYPE,
	.subtokens = tokens_config_dev_type,
	.arg_type = T_ARG_TOKEN,
	.help = "Board name"},
	{T_LIST,
	.help = "Show config parameters"},
	{T_SAVE,
	.help = "Save configured parameters"},
	{T_BOOT,
	.help = "Go to bootloader"},
	{0}};

#if CONFIG_USE_SERIAL_PLOT
t_token tokens_channels_param[] = {
	{T_TEST_CH_1},
	{T_TEST_CH_2},
	{T_TEST_CH_3},
	{0}};

t_token tokens_monitor[] = {
	{T_ON},
	{T_OFF},
	{T_ADD,
	 .subtokens = tokens_channels_param,
	 .arg_type = T_ARG_TOKEN,
	 .help = "Add parameters to channel"},
	{T_RESET,
	 .help = "Remove all channels"},
	{T_PERIOD,
	 .arg_type = T_ARG_UINT,
	 .help = "Period data send [ms]"},
	{T_MONITOR_START_SEQ,
	 .arg_type = T_ARG_UINT,
	 .help = "Monitor start sequence UINT32"},
	{T_LIST,
	 .help = "Write all configured parameters"},
	{0}};

#endif

t_token tokens_echo[] = {
	{T_ON},
	{T_OFF},
	{0}
};

//main menu
t_token tl_tokens[] = {
	{T_HELP,
	 .arg_type = T_ARG_HELP,
	 .help = "Available commands"},
	{T_CLEAR,
	 .help = "Clear screen"},
#if CONFIG_USE_CONSOLE_TOKEN_DEBUG
	{T_DEBUG,
	 .subtokens = tokens_debug,
	 .help = "Debug mode"},
#endif //CONFIG_USE_CONSOLE_TOKEN_DEBUG	 
	{T_CONFIG,
	.subtokens = tokens_config,
	.help = "Configs"},	 
#if CONFIG_USE_SERIAL_PLOT
	{T_MONITOR,
	.subtokens = tokens_monitor,
	.help = "Serial plot monitor"},
#endif
	{T_ECHO,
	.subtokens = tokens_echo,
	.help = "Console echo"},
	#if CONFIG_USE_MQTT
	{T_MQTT,
	.subtokens = tokens_mqtt,
	.help = "MQTT server parametrs"},
	#endif
	{T_RESET,
	.arg_type = T_ARG_UINT,
	.help = "For the reset write 'reset 1'"},
	#if CONFIG_USE_WIFI
	{T_WIFI,
	.subtokens = tokens_wifi,
	.help = "Wifi",},
	#endif
	{0}};
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif //#if CONFIG_USE_CONSOLE_TOKEN
