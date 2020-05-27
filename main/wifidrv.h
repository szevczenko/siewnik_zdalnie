#ifndef _WIFI_DRV_H
#define _WIFI_DRV_H

#include "config.h"
#include "console.h"
#include "token.h"
#include "tokenline.h"

typedef struct
{
  char ssid[33];
  char password[64];
}wifiConData_t;

int configWiFi(console_t *con, t_tokenline_parsed *p);
void wifiDrvInit(void);
int wifiDataSave(wifiConData_t *data);
esp_err_t wifiDataRead(wifiConData_t *data);

void wifiDrvStartScan(void);
void wifiDrvWriteList(console_t *con);
int wifiDrvSetFromAPList(uint8_t num);
int wifiDrvSetAPName(char* name, size_t len);
int wifiDrvSetPassword(char* passwd, size_t len);
int wifiDrvConnect(void);
int wifiStartAccessPoint(void);
int wifiDrvIsConnected(void);

void wifiDrvStartScan(void);
int wifiDrvGetAPName(char* name);
int wifiDrvGetNameFromScannedList(uint8_t number, char * name);
void wifiDrvGetScanResult(uint16_t *ap_count);

#endif