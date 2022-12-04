/*****************************************************************************/
/**
 * @file   scanrig.h
 * @brief  Header file for the Raspberry Pi implementation of the Geolocation
 * Scanning Rig using the Raspberry Pi Operating System.
 *
 * It's required to consult the Geolocation Scanning Rig Software
 * Implementation Guide on how to install the operating system.
 *
 * @author Luke Smith (lukesmith@vt.edu)
 */
/*****************************************************************************/

/***************************** Include Files *********************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

/****************************** Definitions **********************************/

/* System */
#define I2C_SLEEP_TIME 200 * 1000
#define SCAN_SLEEP_TIME 6600 * 1000
#define DEVICE_ID_0 0x08
#define DEVICE_ID_1 0x09
#define DEVICE_ID_2 0x0A
#define DEVICE_ID_3 0x0B
#define NUM_SCAN_MODULES 3
#define I2C_BUS_0 "/dev/i2c-0" // Physical/Board pin 27 (SDA) & 28 (SCL)

/* Buffer capacities and indices */
#define WIFI_STR_LEN 40
#define BLE_STR_LEN 7
#define WIFI_COUNT_MAX 255
#define BLE_COUNT_MAX 255
#define WIFI_COUNT_INDEX 0
#define BLE_COUNT_INDEX 1
#define SCAN_DATA_BUFFER_SIZE                                                 \
  (WIFI_COUNT_MAX * WIFI_STR_LEN) + (BLE_COUNT_MAX * BLE_STR_LEN)
#define SSID_MAX_LEN 32
#define MAC_ADDR_LEN 6
#define USER_INFO_BUFFER_SIZE 128
#define DEBUG_FILENAME_SIZE 27
#define PAYLOAD_FILENAME_SIZE 29

/* Microcontroller states */
#define TEST_BYTE 0x11
#define SCAN_REQUEST 0x12
#define READ_SIGNAL_COUNTS 0x13
#define READ_SIGNAL_DATA 0x14

/* Define output stream parameters */
#define ENABLE_LOG_TO_FILE 1

/****************************** Structures ***********************************/

typedef struct WifiEntry
{
  char tech;
  char ssid[SSID_MAX_LEN + 1];
  char mac[MAC_ADDR_LEN];
  char rssi;
} WifiEntry;

typedef struct BLEEntry
{
  char mac[MAC_ADDR_LEN];
  char rssi;
} BLEEntry;

typedef struct WifiList
{
  WifiEntry entries[WIFI_COUNT_MAX];
  int count;
} WifiList;

typedef struct BLEList
{
  BLEEntry entries[BLE_COUNT_MAX];
  int count;
} BLEList;

typedef struct LocationInfo
{
  char propertyname[USER_INFO_BUFFER_SIZE];
  char poiname[USER_INFO_BUFFER_SIZE];
  long int group;
} LocationInfo;

typedef struct Application
{
  char scan_data_buf[NUM_SCAN_MODULES][SCAN_DATA_BUFFER_SIZE];
  char debug_filename[DEBUG_FILENAME_SIZE];
  char payload_filename[PAYLOAD_FILENAME_SIZE];
  unsigned int starttime;
  FILE *debug_stream;
  int fd[NUM_SCAN_MODULES];
} Application;

/******************************* Constructors ********************************/

WifiEntry
WifiEntry_construct ()
{
  WifiEntry w = { .tech = '~', .ssid = { '~' }, .mac = { '~' }, .rssi = '~' };
  return w;
}

BLEEntry
BLEEntry_construct ()
{
  BLEEntry b = { .mac = { '~' }, .rssi = '~' };
  return b;
}

LocationInfo
LocationInfo_construct ()
{
  LocationInfo info
      = { .propertyname = { '~' }, .poiname = { '~' }, .group = 0 };
  return info;
}

Application
Application_construct ()
{
  Application app = { .scan_data_buf = { { '~' } },
                      .debug_filename = { '~' },
                      .payload_filename = { '~' },
                      .starttime = 0,
                      .debug_stream = stdout,
                      .fd = { 0 } };
  return app;
}

/**************************** Function Prototypes ****************************/

void main_loop (Application *app, LocationInfo *info);
void writeToPayloadFile (LocationInfo *info, WifiList w_list, BLEList b_list,
                         FILE *f);