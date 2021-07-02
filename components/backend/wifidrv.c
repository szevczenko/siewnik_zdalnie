#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "config.h"
#include "wifidrv.h"
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
#include "lwip/apps/httpd.h"
#include "lwip/apps/lwiperf.h"

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
#include "menu.h"
#include "sleep_e.h"


#define DEFAULT_SCAN_LIST_SIZE 16
#define CONFIG_TCPIP_EVENT_THD_WA_SIZE 2048
#define MAX_VAL(a, b) a > b ? a : b
#define LOG(...) debug_msg(__VA_ARGS__)

typedef enum 
{
    WIFI_APP_INIT = 0,
    WIFI_APP_IDLE,
    WIFI_APP_CONNECT,
    WIFI_APP_WAIT_CONNECT,
    WIFI_APP_SCAN,
    WIFI_APP_START,
    WIFI_APP_STOP,
    WIFI_APP_READY,
    WIFI_APP_DEINIT
}wifi_app_status_t;

char *wifi_state_name[] = 
{
  [WIFI_APP_INIT] =         "WIFI_APP_INIT",
  [WIFI_APP_IDLE] =         "WIFI_APP_IDLE",
  [WIFI_APP_CONNECT] =      "WIFI_APP_CONNECT",
  [WIFI_APP_WAIT_CONNECT] = "WIFI_APP_WAIT_CONNECT",
  [WIFI_APP_SCAN] =         "WIFI_APP_SCAN",
  [WIFI_APP_START] =        "WIFI_APP_START",
  [WIFI_APP_STOP] =         "WIFI_APP_STOP",
  [WIFI_APP_READY] =        "WIFI_APP_READY",
  [WIFI_APP_DEINIT] =        "WIFI_APP_DEINIT"
};

typedef struct 
{
  wifi_app_status_t state;
  int retry;
  bool connected;
  bool disconect_req;
  bool connect_req;
  uint32_t connect_attemps;
  uint32_t reason_disconnect;
  wifi_ap_record_t scan_list[DEFAULT_SCAN_LIST_SIZE];
  wifi_config_t wifi_config;
  wifiConData_t wifi_con_data;
} wifidrv_ctx_t;

wifidrv_ctx_t ctx;

wifi_config_t wifi_config_ap = {
  .ap = {
    .password = WIFI_AP_PASSWORD,
    .max_connection = 2,
    .authmode = WIFI_AUTH_WPA_WPA2_PSK
  },
};

static void clientGetServerStatus(void);

static void on_wifi_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    system_event_sta_disconnected_t *event = (system_event_sta_disconnected_t *)event_data;
    ctx.connected = false;
    ctx.reason_disconnect = event->reason;
    tcpip_adapter_down(TCPIP_ADAPTER_IF_STA);
}

static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  switch(event_id)
  {
    case IP_EVENT_STA_GOT_IP:
      ctx.connected = true;
      if (memcmp(ctx.wifi_con_data.ssid, ctx.wifi_config.sta.ssid, MAX_VAL(strlen((char *)ctx.wifi_config.sta.ssid), strlen((char *)ctx.wifi_con_data.ssid))) != 0 || 
          memcmp(ctx.wifi_con_data.password, ctx.wifi_config.sta.password, MAX_VAL(strlen((char *)ctx.wifi_config.sta.password), strlen((char *)ctx.wifi_con_data.password))) != 0)
      {
        strcpy((char *)ctx.wifi_con_data.ssid,(char *) ctx.wifi_config.sta.ssid);
        strcpy((char *)ctx.wifi_con_data.password, (char *)ctx.wifi_config.sta.password);
        wifiDataSave(&ctx.wifi_con_data);
      }
    break;
  }    
}

int wifiDataSave(wifiConData_t *data)
{
	nvs_handle my_handle;
    esp_err_t err;
	 // Open
    err = nvs_open(WIFI_AP_NAME, NVS_READWRITE, &my_handle);
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
  err = nvs_open(WIFI_AP_NAME, NVS_READWRITE, &my_handle);
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
  /* Nadawanie nazwy WiFi Access point oraz przypisanie do niego mac adresu */
  if (config.dev_type == T_DEV_TYPE_SERVER)
  {
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    strcpy((char *)wifi_config_ap.ap.ssid, WIFI_AP_NAME);
    for (int i = 0; i < sizeof(mac); i++) {
      sprintf((char *)&wifi_config_ap.ap.ssid[strlen((char *)wifi_config_ap.ap.ssid)], ":%x", mac[i]);
    }
    wifi_config_ap.ap.ssid_len = strlen((char *)wifi_config_ap.ap.ssid);
  }

  /* Inicjalizacja WiFi */
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &got_ip_event_handler, NULL));

  if (config.dev_type == T_DEV_TYPE_SERVER)
  {
    wifiStartAccessPoint();
    ctx.state = WIFI_APP_READY;
  }
  else
  {
    if (wifiDataRead(&ctx.wifi_con_data) == ESP_OK)
    {
      strcpy((char *)ctx.wifi_config.sta.ssid, (char *)ctx.wifi_con_data.ssid);
      strcpy((char *)ctx.wifi_config.sta.password, (char *)ctx.wifi_con_data.password);
    }
    else
    {
      ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
      ESP_ERROR_CHECK(esp_wifi_start());
      esp_wifi_set_config(ESP_IF_WIFI_STA, &ctx.wifi_config);
    }
    ctx.state = WIFI_APP_IDLE;
  }
  LOG("Wifi init ok\n\r");
}

static void wifi_idle(void)
{
  if (config.dev_type == T_DEV_TYPE_SERVER)
  {
    ctx.state = WIFI_APP_START;
  }
  else
  {
    vTaskDelay(MS2ST(1000));
    if (ctx.connect_req)
    {
      ctx.state = WIFI_APP_CONNECT;
    }
  }
}

static void wifi_connect(void)
{
  int ret = 0;
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
  esp_wifi_set_config(ESP_IF_WIFI_STA, &ctx.wifi_config);
  ret = esp_wifi_connect();
  if (ret == ESP_OK)
  {
    ctx.state = WIFI_APP_WAIT_CONNECT;
  }
  else
  {
    LOG("Internal error connect %d attemps %d", ret, ctx.connect_attemps);
    ctx.state = WIFI_APP_IDLE;
    if(ctx.connect_attemps > 3)
    {
      ctx.connect_attemps = 0;
      ctx.connect_req = false;
      esp_wifi_stop();
    } 
    ctx.connect_attemps++;
  }
}

static void wifi_wait_connect(void)
{
  if (ctx.connected)
  {
    ctx.connect_req = false;
    ctx.connect_attemps = 0;
    ctx.state = WIFI_APP_START;
  }
  else
  {
    if (ctx.connect_attemps > 10)
    {
      LOG("Timeout connect");
      ctx.connect_req = false;
      ctx.connect_attemps = 0;
      ctx.state = WIFI_APP_IDLE;
      esp_wifi_disconnect();
      esp_wifi_stop();
    }
    vTaskDelay(MS2ST(100));
    ctx.connect_attemps++;
  }
}

static void wifi_app_start(void)
{
  if (config.dev_type == T_DEV_TYPE_SERVER)
  {
    #if CONFIG_USE_CONSOLE_TELNET
    telnetStart();
    #endif
    cmdServerStart();
  }
  else
  {
    cmdClientStart();
    if (is_sleeping())
    {
      clientGetServerStatus();
    }
  }
  ctx.state = WIFI_APP_READY;
}

static void wifi_app_stop(void)
{
  if (config.dev_type == T_DEV_TYPE_SERVER)
  {
    #if CONFIG_USE_CONSOLE_TELNET
    telnetStart();
    #endif
    cmdServerStop();
  }
  else
  {
    cmdClientStop();
  }
  if (ctx.disconect_req)
  {
    ctx.disconect_req = false;
    if (ctx.reason_disconnect != 0)
    {
      LOG("Disconnect reason %d", ctx.reason_disconnect);
      ctx.reason_disconnect = 0;
    }
  }
  esp_wifi_disconnect();
  ctx.connected = 0;
  ctx.state = WIFI_APP_IDLE;
}

static void wifi_app_ready(void)
{
  if (ctx.disconect_req || !ctx.connected || ctx.connect_req)
  {
    ctx.state = WIFI_APP_STOP;
  }
  vTaskDelay(MS2ST(200));
}

void wifiDrvStartScan(void)
{
  if (ctx.state != WIFI_APP_IDLE)
    return;
  ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
}

void wifiDrvGetScanResult(uint16_t *ap_count)
{
  uint16_t number = DEFAULT_SCAN_LIST_SIZE;
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ctx.scan_list));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(ap_count));
}

int wifiDrvGetNameFromScannedList(uint8_t number, char * name)
{
  strcpy(name, (char*)ctx.scan_list[number].ssid);
  return TRUE;
}

int wifiDrvSetFromAPList(uint8_t num)
{
  if (num > DEFAULT_SCAN_LIST_SIZE) return 0;
  strcpy((char *)ctx.wifi_config.sta.ssid, (char *)ctx.scan_list[num].ssid);
  return 1;
}

int wifiDrvSetAPName(char* name, size_t len)
{
  if (len > sizeof(ctx.wifi_config.sta.ssid)) return 0;
  memcpy(ctx.wifi_config.sta.ssid, name, len);
  return 1;
}

int wifiDrvGetAPName(char* name)
{
  strcpy(name, (char *)ctx.wifi_config.sta.ssid);
  return TRUE;
}

int wifiDrvSetPassword(char* passwd, size_t len)
{
  strcpy((char *)ctx.wifi_config.sta.password, passwd);
  return 1;
}

int wifiDrvConnect(void)
{
  if (config.dev_type == T_DEV_TYPE_SERVER)
  {
    return -1;
  }
  ctx.connect_req = true;
  return 1;
}

int wifiDrvDisconnect(void)
{
  if (config.dev_type == T_DEV_TYPE_SERVER)
  {
    return -1;
  }
  ctx.disconect_req = true;
  return 1;
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
  return ctx.connected;
}

static void clientGetServerStatus(void) {
  /* Wait to connect to server */
  uint32_t time_to_connect = 0;
  while(cmdClientIsConnected() == 0) {
    if (time_to_connect < 10) {
      time_to_connect++;
      osDelay(275);
    }
    else {
      debug_msg("Timeout connected client to server\n\r");
      return;
    }
  }

  uint32_t start_status = 0;
  time_to_connect = 0;
  /* Get start status */
  while(cmdClientGetValue(MENU_START_SYSTEM, &start_status, 1250) == 0) {
    if (time_to_connect < 4) {
      time_to_connect++;
      osDelay(500);
    }
    else {
      debug_msg("Timeout get MENU_START_SYSTEM\n\r");
      return;
    }
  }

  if (start_status == 0) {
    debug_msg("WifDrv: System not started\n\r");
    return;
  }
  /* Get all data from server */
  if (cmdClientGetAllValue(2500) == 0) {
    debug_msg("Timeout get ALL VALUES\n\r");
    return;
  }
  /* Go to start menu */
  debug_msg("WifDrv: GO To START MENU\n\r");
  taskENTER_CRITICAL();
  menuEnterStartFromServer();
  taskEXIT_CRITICAL();
}

static void wifi_event_task(void * pv)
{ 
  uint32_t prev_state = 0;
	while(1)
	{
    // if (ctx.retry > 10) {
    //   stastus_reg_eth = 0;
    //   esp_wifi_stop();
    //   osDelay(1000);
    //   wifiDrvConnect();
    //   ctx.retry = 0;
    // }
    if (prev_state != ctx.state)
    {
      LOG("Wifi state %s\n\r", wifi_state_name[ctx.state]);
      prev_state = ctx.state;
    }
    
    switch (ctx.state)
    {
      case WIFI_APP_INIT:
        wifi_init();
        break;
      
      case WIFI_APP_IDLE:
        wifi_idle();
        break;

      case WIFI_APP_CONNECT:
        wifi_connect();
        break;

      case WIFI_APP_WAIT_CONNECT:
        wifi_wait_connect();
        break;

      case WIFI_APP_START:
        wifi_app_start();
        break;

      case WIFI_APP_STOP:
        wifi_app_stop();
        break;

      case WIFI_APP_READY:
        wifi_app_ready();
        break;

      case WIFI_APP_DEINIT:
        wifi_app_stop();
        break;

      default:
        ctx.state = WIFI_APP_IDLE;
    }
  }
}

void wifiDrvInit(void)
{
  if (config.dev_type == T_DEV_TYPE_SERVER)
  {
    #if CONFIG_USE_CONSOLE_TELNET
    telnetInit();
    telnetStartTask();
    #endif
    cmdServerStartTask();
  }
  else
  {
    cmdClientStartTask();
  }
  
  xTaskCreate(wifi_event_task, "wifi_event_task", CONFIG_TCPIP_EVENT_THD_WA_SIZE, NULL, NORMALPRIO, NULL);
}

#endif
