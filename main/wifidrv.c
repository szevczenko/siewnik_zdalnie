#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "config.h"
#if (CONFIG_USE_TCPIP)

#include "netif/ethernet.h"

#include "lwip.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "libtelnet.h"
#include "telnet.h"
#include "wifidrv.h"

#include "lwip/apps/httpd.h"
#include "lwip/apps/lwiperf.h"
#include "console_telnet.h"


#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "cmd_server.h"
#include "cmd_client.h"
#include "token.h"

#define DEFAULT_SCAN_LIST_SIZE 16

#define STORAGE_NAMESPACE "Zefir"
#define WIFI_AP_PASSWORD "12345678"

static uint32_t stastus_reg_eth;

static wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
static wifi_config_t wifi_config;

wifi_config_t wifi_config_ap = {
  .ap = {
    .ssid = STORAGE_NAMESPACE,
    .ssid_len = strlen(STORAGE_NAMESPACE),
    .password = WIFI_AP_PASSWORD,
    .max_connection = 2,
    .authmode = WIFI_AUTH_WPA_WPA2_PSK
  },
};

static wifiConData_t wifiData;

#define CONFIG_TCPIP_EVENT_THD_WA_SIZE 2048

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
  switch(event_id)
  {
    case WIFI_EVENT_STA_START:
    break;
    case WIFI_EVENT_STA_CONNECTED:
    break;
    case WIFI_EVENT_STA_DISCONNECTED:
      esp_wifi_connect();
      stastus_reg_eth = 0;
    break;
    case WIFI_EVENT_AP_STADISCONNECTED:
    stastus_reg_eth = 0;
    break;

  }

}
#define MAX_VAL(a, b) a > b ? a : b
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
  switch(event_id)
  {
    case IP_EVENT_STA_GOT_IP:
      stastus_reg_eth = 1;
      if (memcmp(wifiData.ssid, wifi_config.sta.ssid, MAX_VAL(strlen((char *)wifi_config.sta.ssid), strlen((char *)wifiData.ssid))) != 0 || 
          memcmp(wifiData.password, wifi_config.sta.password, MAX_VAL(strlen((char *)wifi_config.sta.password), strlen((char *)wifiData.password))) != 0)
      {
        strcpy((char *)wifiData.ssid,(char *) wifi_config.sta.ssid);
        strcpy((char *)wifiData.password, (char *)wifi_config.sta.password);
        wifiDataSave(&wifiData);
      }
    break;
      case IP_EVENT_STA_LOST_IP:
    break;
    case IP_EVENT_AP_STAIPASSIGNED:
      stastus_reg_eth = 1;
    break;
  }
    
    
}

int wifiDataSave(wifiConData_t *data)
{
	nvs_handle my_handle;
    esp_err_t err;
	 // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
		nvs_close(my_handle);
		return err;
  }

	err = nvs_set_blob(my_handle, "wifi", data, sizeof(wifiConData_t));

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

esp_err_t wifiDataRead(wifiConData_t *data)
{
	nvs_handle my_handle;
  esp_err_t err;
	// Open
  err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) return err;
	// Read the size of memory space required for blob
  size_t required_size = 0;  // value will default to 0, if not set yet in NVS
  err = nvs_get_blob(my_handle, "wifi", NULL, &required_size);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) 
	{
		nvs_close(my_handle);
		return err;
	}

  // Read previously saved blob if available
  if (required_size == sizeof(wifiConData_t)) {
    err = nvs_get_blob(my_handle, "wifi", data, &required_size);
	  nvs_close(my_handle);
    return err;
  }
	nvs_close(my_handle);
	return ESP_ERR_NVS_NOT_FOUND;
}

static void wifi_init(void)
{
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &got_ip_event_handler, NULL));
  if (config.dev_type == T_DEV_TYPE_SERVER)
  {
    
    wifiStartAccessPoint();
  }
  else
  {
    if (wifiDataRead(&wifiData) == ESP_OK)
    {
      strcpy((char *)wifi_config.sta.ssid, (char *)wifiData.ssid);
      strcpy((char *)wifi_config.sta.password, (char *)wifiData.password);
      //wifiDrvConnect();
    }
    else
    {
      ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
      ESP_ERROR_CHECK(esp_wifi_start());
      esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    }
    
  }
}

void wifiDrvStartScan(void)
{
  ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
}

void wifiDrvGetScanResult(uint16_t *ap_count)
{
  uint16_t number = DEFAULT_SCAN_LIST_SIZE;
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(ap_count));
}

void wifiDrvWriteList(console_t *con)
{
  (void) con;
  uint16_t ap_count = 0;
  memset(ap_info, 0, sizeof(ap_info));
  wifiDrvStartScan();
  wifiDrvGetScanResult(&ap_count);
  for (uint8_t i = 0; i < ap_count; i++)
  {
    consolePrintfTimeout(con, CONFIG_CONSOLE_TIMEOUT, "%d. %s strength %d\n\r", i, ap_info[i].ssid, ap_info[i].rssi);
  }
}

int wifiDrvGetNameFromScannedList(uint8_t number, char * name)
{
  strcpy(name, (char*)ap_info[number].ssid);
  return TRUE;
}

int wifiDrvSetFromAPList(uint8_t num)
{
  if (num > DEFAULT_SCAN_LIST_SIZE) return 0;
  strcpy((char *)wifi_config.sta.ssid, (char *)ap_info[num].ssid);
  return 1;
}

int wifiDrvSetAPName(char* name, size_t len)
{
  if (len > sizeof(wifi_config.sta.ssid)) return 0;
  memcpy(wifi_config.sta.ssid, name, len);
  return 1;
}

int wifiDrvGetAPName(char* name)
{
  strcpy(name, (char *)wifi_config.sta.ssid);
  return TRUE;
}

int wifiDrvSetPassword(char* passwd, size_t len)
{
  strcpy((char *)wifi_config.sta.password, passwd);
  return 1;
}

int wifiDrvConnect(void)
{
  if (config.dev_type == T_DEV_TYPE_SERVER)
  {
    return -1;
  }
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
  esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
  return esp_wifi_connect();
}

int wifiDrvDisconnect(void)
{
  if (config.dev_type == T_DEV_TYPE_SERVER)
  {
    return -1;
  }
  stastus_reg_eth = 0;
  return esp_wifi_disconnect();
}

int wifiStartAccessPoint(void)
{
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap));
  ESP_ERROR_CHECK(esp_wifi_start());
  return 1;
}

int wifiDrvIsConnected(void)
{
  return stastus_reg_eth;
}

static void printList(console_t *con)
{
  tcpip_adapter_ip_info_t ip;
  memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
  consolePrintfTimeout(con, CONFIG_CONSOLE_TIMEOUT, "AP Name: %s\n\r", (char *)wifi_config.sta.ssid);
  consolePrintfTimeout(con, CONFIG_CONSOLE_TIMEOUT, "password: %s\n\r", (char *)wifi_config.sta.password);
  if (tcpip_adapter_get_ip_info(ESP_IF_WIFI_STA, &ip) == 0) {
    consolePrintfTimeout(con, CONFIG_CONSOLE_TIMEOUT, "IP: %d.%d.%d.%d\n\r", IP2STR(&ip.ip));
    consolePrintfTimeout(con, CONFIG_CONSOLE_TIMEOUT, "MASK: %d.%d.%d.%d\n\r", IP2STR(&ip.netmask));
    consolePrintfTimeout(con, CONFIG_CONSOLE_TIMEOUT, "GW: %d.%d.%d.%d\n\r", IP2STR(&ip.gw));
  }
}

int configWiFi(console_t *con, t_tokenline_parsed *p)
{
  switch (p->tokens[1])
	{
    case T_SSID:
			wifiDrvSetAPName(p->buf, strlen(p->buf));
      return TRUE;
		break;
		case T_SSID_NUMBER:
      wifiDrvSetFromAPList((uint8_t)*p->buf);
      return TRUE;
		break;
		case T_PASSWORD:
      wifiDrvSetPassword(p->buf, strlen(p->buf));
      return TRUE;
		break;
    case T_CONNECT:
      wifiDrvConnect();
      return TRUE;
    break;
    case T_SHOW:
      wifiDrvWriteList(con);
      return TRUE;
    break;
    case T_LIST:
      printList(con);
      return TRUE;
    break;
		default:
			return FALSE;
		break;
  }
  return 1;

}

static void wifi_event_task(void * pv)
{ 
  wifi_init();
  vTaskDelay(MS2ST(1000));
  uint32_t previos_st_eth = 0;
  #if CONFIG_USE_IPERF
  lwiperf_start_tcp_server_default(lwiperf_report, NULL);
  #endif
	while(1)
	{
		if(stastus_reg_eth != previos_st_eth)
		{
			if (stastus_reg_eth)
			{
        #if CONFIG_USE_CONSOLE_TELNET
        telnetStart();
        #endif
        #if CONFIG_USE_MQTT
        MQTTStart();
        #endif
        if (config.dev_type == T_DEV_TYPE_SERVER)
        {
          cmdServerStart();
        }
        else
        {
          cmdClientStart();
        }
        
			}
			else
			{
        #if CONFIG_USE_CONSOLE_TELNET
        telnetStop();
        #endif
        #if CONFIG_USE_MQTT
        MQTTStop();
        #endif
        if (config.dev_type == T_DEV_TYPE_SERVER)
        {
          cmdServerStop();
        }
        else
        {
          cmdClientStop();
        }
        
			}
			previos_st_eth = stastus_reg_eth;
		}
		osDelay(1000);
	}
}

void wifiDrvInit(void)
{
  #if CONFIG_USE_CONSOLE_TELNET
  telnetInit();
  telnetStartTask();
  consoleEthInit();
  consoleTelentStart();
  #endif
  #if CONFIG_USE_MQTT
	MQTT_create_task();
	#endif
  if (config.dev_type == T_DEV_TYPE_SERVER)
  {
    cmdServerStartTask();
  }
  else
  {
    cmdClientStartTask();
  }
  
  xTaskCreate(wifi_event_task, "wifi_event_task", CONFIG_TCPIP_EVENT_THD_WA_SIZE, NULL, NORMALPRIO, NULL);
}

#endif
