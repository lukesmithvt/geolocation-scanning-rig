#include <Arduino.h>
#include <Wire.h>

#include "BLEDevice.h"
#include <lwip_netconf.h>
// C:\Users\Luke\AppData\Local\Arduino15\packages\realtek\hardware\AmebaD\3.1.4\system\component\common\api\wifi\wifi_conf.h
#include <wifi_conf.h>
#include <wifi_constants.h>
#include <wifi_structures.h>
// C:\Users\Luke\AppData\Local\Arduino15\packages\realtek\hardware\AmebaD\3.1.4\cores\ambd\wl_definitions.h
#include <wl_definitions.h>
#include <wl_types.h>

#define DEVICE_ID 0x08 // 0x09 0x0A 0x0B

#define WIFI_SCAN_INTERVAL 500
#define BLE_SCAN_INTERVAL 1000
#define RSSI_CEILING -30
#define RSSI_FLOOR -100
#define SSID_MAX_LEN WL_SSID_MAX_LENGTH
#define MAC_ADDR_LEN WL_MAC_ADDR_LENGTH
#define WIFI_STR_LEN 40
#define BLE_STR_LEN 7
#define WIFI_COUNT_MAX WL_NETWORKS_LIST_MAXNUM
#define BLE_COUNT_MAX 256
#define BUFFER_SIZE                                                           \
  (WIFI_COUNT_MAX * WIFI_STR_LEN) + (BLE_COUNT_MAX * BLE_STR_LEN)

#define SCAN_REQUEST 0x11
#define READ_SIGNAL_COUNTS 0x12
#define READ_SIGNAL_DATA 0x14
#define TEST_BYTE 0x05

static char print_buf[WIFI_STR_LEN];
unsigned char WriteBuffer[BUFFER_SIZE];
// BLEAdvertData foundDevice;

static uint8_t wifi_count;
static uint8_t ble_count;
unsigned char signal_counts[2];
int state;

static rtw_result_t
wifidrv_scan_result_handler (rtw_scan_handler_result_t *malloced_scan_result)
{
  rtw_scan_result_t *record;

  if (malloced_scan_result->scan_complete != RTW_TRUE)
    {
      record = &malloced_scan_result->ap_details;
      record->SSID.val[record->SSID.len]
          = 0; /* Ensure the SSID is null terminated */

      if (wifi_count < WIFI_COUNT_MAX)
        {
          unsigned char *p = &WriteBuffer[wifi_count * WIFI_STR_LEN];
          *p++ = record->channel < 14 ? '2' : '5';
          for (int i = 0; i < SSID_MAX_LEN; i++)
            {
              *p++ = record->SSID.val[i];
            }
          for (int i = 0; i < MAC_ADDR_LEN; i++)
            {
              *p++ = record->BSSID.octet[i];
            }
          *p++ = (char)(record->signal_strength & 0xFF);
          wifi_count++;
        }
    }

  return RTW_SUCCESS;
}

void
scanFunction (T_LE_CB_DATA *p_data)
{
  // BLE.configScan()->printScanInfo(p_data);
  // foundDevice.parseScanInfo (p_data);
  T_LE_SCAN_INFO *scan_info = p_data->p_le_scan_info;

  unsigned char *p
      = &WriteBuffer[(wifi_count * WIFI_STR_LEN) + (ble_count * BLE_STR_LEN)];
  for (int i = MAC_ADDR_LEN - 1; i >= 0; i--)
    {
      // *p++ = (unsigned char)foundDevice.getAddr ().data ()[i];
      *p++ = (unsigned char)scan_info->bd_addr[i];
    }
  // *p++ = (unsigned char)foundDevice.getRSSI ();
  *p++ = (unsigned char)scan_info->rssi;
  ble_count++;
}

static int8_t
scanNetworks ()
{
  uint8_t attempts = 10;

  if (wifi_scan_networks (wifidrv_scan_result_handler, NULL) != RTW_SUCCESS)
    {
      return WL_FAILURE;
    }

  do
    {
      delay (WIFI_SCAN_INTERVAL);
    }
  while ((wifi_count == 0) && (--attempts > 0));
  return wifi_count;
}

void
requestEvent ()
{

  switch (state)
    {
    case READ_SIGNAL_COUNTS:
      {
        signal_counts[0] = wifi_count;
        signal_counts[1] = ble_count;
        Wire.slaveWrite (signal_counts, 2);
        state = READ_SIGNAL_DATA;
        break;
      }
    case READ_SIGNAL_DATA:
      {
        int write_len
            = (wifi_count * WIFI_STR_LEN) + (ble_count * BLE_STR_LEN);
        Serial.println ("Sending scan data...");
        int starttime = millis ();
        Wire.slaveWrite (WriteBuffer, write_len);
        int delta = millis () - starttime;
        sprintf (print_buf, "Finished in %d ms.", delta);
        Serial.println (print_buf);

        for (int i = 0; i < write_len; i++)
          {
            if (i % WIFI_STR_LEN == 0)
              {
                Serial.println ();
              }
            sprintf (print_buf, "%02X ", WriteBuffer[i]);
            Serial.print (print_buf);
          }
        break;
      }
    default:
      Serial.println ("Initiated request on invalid state.");
      break;
    }
}

void
receiveEvent (int num_bytes)
{
  (void)num_bytes;
  state = Wire.read ();

  switch (state)
    {
    case SCAN_REQUEST:
      {
        wifi_count = 0;
        ble_count = 0;
        Serial.println ("Conducting scan...");
        int starttime = millis ();
        scanNetworks ();
        BLE.configScan ()->startScan (BLE_SCAN_INTERVAL);
        int delta = millis () - starttime;

        state = READ_SIGNAL_COUNTS;

        sprintf (print_buf, "Found %d wifi networks and %d ble devices.",
                 wifi_count, ble_count);
        Serial.println (print_buf);
        sprintf (print_buf, "Finished in %d ms.", delta);
        Serial.println (print_buf);
        break;
      }
    case TEST_BYTE:
      {
        Serial.println ("Test...");
        break;
      }
    default:
      Serial.println ("Invalid state received.");
      break;
    }
}

void
setup ()
{
  Serial.begin (115200);
  LwIP_Init ();
  wifi_on (RTW_MODE_STA);
  Wire.begin (DEVICE_ID);
  Wire.onRequest (requestEvent);
  Wire.onReceive (receiveEvent);

  BLE.init ();
  BLE.configScan ()->setScanMode (GAP_SCAN_MODE_ACTIVE);
  BLE.setScanCallback (scanFunction);
  BLE.beginCentral (0);

  wifi_count = 0;
  ble_count = 0;
}

void
loop ()
{
}
