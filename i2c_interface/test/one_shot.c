#include "scanrig.h"

int
main (int argc, char **argv)
{
  if (argc < 2)
    {
      printf ("Usage: %s <num_of_scans>\n", argv[0]);
      exit (1);
    }

  Application app = Application_construct ();

  for (int i = 0; i < strtol (argv[1], NULL, 10); i++)
    main_loop (&app);
  return 0;
}

void
main_loop (Application *app)
{

  /* Start stopwatch */
  app->starttime = millis ();

  /* Generate timestamp */
  struct timespec ts;
  clock_gettime (CLOCK_MONOTONIC_RAW, &ts);
  unsigned long long timestamp = (time (NULL) << 32) + ts.tv_nsec;

  /* Setup debug output*/
#ifdef DEBUG_TO_FILE
  snprintf (app->debug_filename, DEBUG_FILENAME_SIZE, "./log/%llx.log",
            timestamp);
  if ((app->debug_stream = fopen (app->debug_filename, "w")) == NULL)
    {
      fprintf (stdout, "Failed to open log file for writing.\n");
      exit (1);
    }
#else
  app->debug_stream = stdout;
#endif

  /* Setup I2C communication */
  app->fd[0] = wiringPiI2CSetupInterface ("/dev/i2c-0", DEVICE_ID_0);
  if (app->fd[0] == -1)
    {
      fprintf (app->debug_stream, "Failed to init I2C communication.\n");
      exit (1);
    }
  fprintf (app->debug_stream, "I2C communication successfully setup.\n");

  /* Test connection */
  fprintf (app->debug_stream, "[%d] Testing connection\n",
           millis () - app->starttime);
    while (wiringPiI2CWrite (app->fd[0], TEST_BYTE) < 0)
    {
      usleep (I2C_SLEEP_TIME);
    }

  /* Send scan signal */
  usleep (I2C_SLEEP_TIME);
  fprintf (app->debug_stream, "[%d] Sending scan signal\n",
           millis () - app->starttime);
  wiringPiI2CWrite (app->fd[0], SCAN_REQUEST);

  /* Request signal counts */
  int wifi_count;
  int ble_count;

  usleep (I2C_SLEEP_TIME);
  fprintf (app->debug_stream, "[%d] Requesting signal counts\n",
           millis () - app->starttime);
  usleep (SCAN_SLEEP_TIME);
  while ((wifi_count = wiringPiI2CRead (app->fd[0])) < 0)
    {
      fprintf (app->debug_stream, ".");
      usleep (I2C_SLEEP_TIME);
    }

  while ((ble_count = wiringPiI2CRead (app->fd[0])) < 0)
    {
      fprintf (app->debug_stream, ".");
      usleep (I2C_SLEEP_TIME);
    }
  fprintf (app->debug_stream, "Found %d wifi networks and %d ble devices\n",
           wifi_count, ble_count);

  /* Request signal data */
  fprintf (app->debug_stream, "[%d] Requesting signal data\n",
           millis () - app->starttime);
  int read_len = (wifi_count * WIFI_STR_LEN) + (ble_count * BLE_STR_LEN);
  while (read (app->fd[0], app->scan_data_buf, read_len) < 0)
    {
      fprintf (app->debug_stream, ".");
      usleep (I2C_SLEEP_TIME);
    }

  /* Organize signal data for writing to file*/
  fprintf (app->debug_stream, "[%d] Parsing signal data\n",
           millis () - app->starttime);
  char *p = &app->scan_data_buf[0];

  WifiList w_list
      = { .entries = { WifiEntry_construct () }, .count = wifi_count };

  for (int i = 0; i < w_list.count; i++)
    {
      WifiEntry w = WifiEntry_construct ();

      w.tech = *p++;
      for (int j = 0; j < 32; j++)
        {
          w.ssid[j] = *p++;
        }
      w.ssid[32] = '\0';
      for (int j = 0; j < 6; j++)
        {
          w.mac[j] = *p++;
        }
      w.rssi = *p++;

      w_list.entries[i] = w;
    }

  BLEList b_list
      = { .entries = { BLEEntry_construct () }, .count = ble_count };

  for (int i = 0; i < b_list.count; i++)
    {
      BLEEntry b = BLEEntry_construct ();

      for (int j = 0; j < 6; j++)
        {
          b.mac[j] = *p++;
        }
      b.rssi = *p++;

      b_list.entries[i] = b;
    }

  /* Create and write signal data to file */
  fprintf (app->debug_stream, "[%d] Creating payload\n",
           millis () - app->starttime);
  snprintf (app->payload_filename, PAYLOAD_FILENAME_SIZE, "./raw/%llx.json",
            timestamp);
  FILE *f_out;
  if ((f_out = fopen (app->payload_filename, "w")) == NULL)
    {
      fprintf (app->debug_stream, "Failed to open file for writing.\n");
      exit (1);
    }
  writeToPayloadFile (w_list, b_list, f_out);

  /* Stop stopwatch */
  fprintf (app->debug_stream, "[%d] Complete\n", millis () - app->starttime);
}

void
writeToPayloadFile (WifiList w_list, BLEList b_list, FILE *f)
{

  /* Header */
  fprintf (f, "{\n\
\t\"timestamp\": %ld,\n\
\t\"propertyName\": \"%s\",\n\
\t\"poiName\": \"%s\",\n\
\t\"group\": %d,\n\
\t\"scanData\": [\n\
",
           time (NULL), "My Apartment", "Bedroom", 1337);

  /* Write all captured wifi signal data */
  for (int i = 0; i < w_list.count; i++)
    {
      WifiEntry w = w_list.entries[i];

      fprintf (f, "\t\t{\n");
      fprintf (f, "\t\t\t\"tech\": \"wifi-%s\",\n",
               w.tech == '2' ? "2.4" : "5");
      fprintf (f, "\t\t\t\"ssid\": \"%s\",\n", w.ssid);
      fprintf (f, "\t\t\t\"mac\": \"%02x%02x%02x%02x%02x%02x\",\n", w.mac[0],
               w.mac[1], w.mac[2], w.mac[3], w.mac[4], w.mac[5]);
      fprintf (f, "\t\t\t\"count\": %u,\n", 1);
      fprintf (f, "\t\t\t\"rssi_raw\": %hhd,\n", w.rssi);
      fprintf (f, "\t\t\t\"rssi_min\": %hhd,\n", w.rssi);
      fprintf (f, "\t\t\t\"rssi_max\": %hhd,\n", w.rssi);
      fprintf (f, "\t\t\t\"rssi_avg\": %hhd\n", w.rssi);

      fprintf (f, "\t\t%s\n",
               (i == w_list.count - 1) && (b_list.count == 0) ? "}" : "},");
    }

  /* Write all captured ble signal data */
  for (int i = 0; i < b_list.count; i++)
    {
      BLEEntry b = b_list.entries[i];

      fprintf (f, "\t\t{\n");
      fprintf (f, "\t\t\t\"tech\": \"ble\",\n");
      fprintf (f, "\t\t\t\"ssid\": \"\",\n");
      fprintf (f, "\t\t\t\"mac\": \"%02x%02x%02x%02x%02x%02x\",\n", b.mac[0],
               b.mac[1], b.mac[2], b.mac[3], b.mac[4], b.mac[5]);
      fprintf (f, "\t\t\t\"count\": %u,\n", 1);
      fprintf (f, "\t\t\t\"rssi_raw\": %hhd,\n", b.rssi);
      fprintf (f, "\t\t\t\"rssi_min\": %hhd,\n", b.rssi);
      fprintf (f, "\t\t\t\"rssi_max\": %hhd,\n", b.rssi);
      fprintf (f, "\t\t\t\"rssi_avg\": %hhd\n", b.rssi);

      fprintf (f, "\t\t%s\n", i == b_list.count - 1 ? "}" : "},");
    }

  /* Closing Brackets */
  fprintf (f, "\n\t]\n}");
}