#include "scanrig.h"

int
main (int argc, char **argv)
{
  if (argc < 5)
    {
      printf ("Usage: %s <num scans> <propertyName> <poiName> <group>\n",
              argv[0]);
      exit (1);
    }

  Application app = Application_construct ();
  LocationInfo info = LocationInfo_construct ();
  strcpy (info.propertyname, argv[2]);
  strcpy (info.poiname, argv[3]);
  info.group = strtol (argv[4], NULL, 10);

  for (int i = 0; i < strtol (argv[1], NULL, 10); i++)
    main_loop (&app, &info);
  return 0;
}

void
main_loop (Application *app, LocationInfo *info)
{

  /* Start stopwatch */
  app->starttime = millis ();

  /* Generate timestamp */
  struct timespec ts;
  clock_gettime (CLOCK_MONOTONIC_RAW, &ts);
  unsigned long long timestamp = (time (NULL) << 32) + ts.tv_nsec;

  /* Setup debug output*/
#if ENABLE_LOG_TO_FILE
  snprintf (app->debug_filename, DEBUG_FILENAME_SIZE, "./log/%llx.log",
            timestamp);
  mkdir ("./log", 0777);
  if ((app->debug_stream = fopen (app->debug_filename, "w")) == NULL)
    {
      fprintf (stdout, "Failed to open log file for writing.\n");
      exit (1);
    }
#else
  app->debug_stream = stdout;
#endif

  /* Setup I2C communication */
  for (int i = 0; i < NUM_SCAN_MODULES; i++)
    {
      app->fd[i] = wiringPiI2CSetupInterface (I2C_BUS_0, DEVICE_ID_0 + i);
      if (app->fd[i] == -1)
        {
          fprintf (app->debug_stream,
                   "Failed to init I2C communication with device ID: %d.\n",
                   DEVICE_ID_0 + i);
          exit (1);
        }
    }

  /* Test connection */
  fprintf (app->debug_stream, "[%d] Testing connection\n",
           millis () - app->starttime);
  fflush (app->debug_stream);
  for (int i = 0; i < NUM_SCAN_MODULES; i++)
    {
      wiringPiI2CWrite (app->fd[i], TEST_BYTE);
      usleep(I2C_SLEEP_TIME);
    }

  /* Send scan signal */
  usleep (I2C_SLEEP_TIME);
  fprintf (app->debug_stream, "[%d] Sending scan signal\n",
           millis () - app->starttime);
  fflush (app->debug_stream);
  for (int i = 0; i < NUM_SCAN_MODULES; i++)
    {
      wiringPiI2CWrite (app->fd[i], SCAN_REQUEST);
      usleep(I2C_SLEEP_TIME);
    }

  /* Request signal counts */
  int wifi_count[NUM_SCAN_MODULES];
  int ble_count[NUM_SCAN_MODULES];

  fprintf (app->debug_stream, "[%d] Requesting signal counts\n",
           millis () - app->starttime);
  fflush (app->debug_stream);
  usleep (SCAN_SLEEP_TIME);
  for (int i = 0; i < NUM_SCAN_MODULES; i++)
    {
      while ((wifi_count[i] = wiringPiI2CRead (app->fd[i])) < 0)
        {
          usleep (I2C_SLEEP_TIME);
        }
      while ((ble_count[i] = wiringPiI2CRead (app->fd[i])) < 0)
        {
          usleep (I2C_SLEEP_TIME);
        }
      fprintf (app->debug_stream,
               "[%d] Module at address %d found %d wifi networks and %d ble "
               "devices\n",
               millis () - app->starttime, DEVICE_ID_0 + i, wifi_count[i],
               ble_count[i]);
      fflush (app->debug_stream);
    }

  /* Request signal data */
  fprintf (app->debug_stream, "[%d] Requesting signal data\n",
           millis () - app->starttime);
  fflush (app->debug_stream);
  for (int i = 0; i < NUM_SCAN_MODULES; i++)
    {
      int read_len
          = (wifi_count[i] * WIFI_STR_LEN) + (ble_count[i] * BLE_STR_LEN);

      while (read (app->fd[i], app->scan_data_buf[i], read_len) < 0)
        {
          fprintf (app->debug_stream, ".");
          usleep (I2C_SLEEP_TIME);
        }
    }

  /* Organize signal data for writing to file*/
  fprintf (app->debug_stream, "[%d] Parsing signal data\n",
           millis () - app->starttime);
  fflush (app->debug_stream);

  for (int i = 0; i < NUM_SCAN_MODULES; i++)
    {
      char *p = &app->scan_data_buf[i][0];

      WifiList w_list
          = { .entries = { WifiEntry_construct () }, .count = wifi_count[i] };

      for (int j = 0; j < w_list.count; j++)
        {
          WifiEntry w = WifiEntry_construct ();

          w.tech = *p++;
          for (int k = 0; k < SSID_MAX_LEN; k++)
            {
              w.ssid[k] = *p++;
            }
          w.ssid[SSID_MAX_LEN] = '\0';
          for (int k = 0; k < MAC_ADDR_LEN; k++)
            {
              w.mac[k] = *p++;
            }
          w.rssi = *p++;

          w_list.entries[j] = w;
        }

      BLEList b_list
          = { .entries = { BLEEntry_construct () }, .count = ble_count[i] };

      for (int j = 0; j < b_list.count; j++)
        {
          BLEEntry b = BLEEntry_construct ();

          for (int k = 0; k < MAC_ADDR_LEN; k++)
            {
              b.mac[k] = *p++;
            }
          b.rssi = *p++;

          b_list.entries[j] = b;
        }

      /* Create and write signal data to file */
      fprintf (app->debug_stream,
               "[%d] Creating payload for module at address %d\n",
               millis () - app->starttime, DEVICE_ID_0 + i);
      fflush (app->debug_stream);
      snprintf (app->payload_filename, PAYLOAD_FILENAME_SIZE,
                "./raw/%llx.json", timestamp);
      FILE *f_out;
      mkdir ("./raw", 0777);
      if ((f_out = fopen (app->payload_filename, "w")) == NULL)
        {
          fprintf (app->debug_stream,
                   "Failed to open payload file for writing.\n");
          exit (1);
        }
      writeToPayloadFile (info, w_list, b_list, f_out);
      fclose (f_out);
    }

  /* Stop stopwatch */
  fprintf (app->debug_stream, "[%d] Payload complete\n",
           millis () - app->starttime);
  fflush (app->debug_stream);
}

void
writeToPayloadFile (LocationInfo *info, WifiList w_list, BLEList b_list,
                    FILE *f)
{

  /* Header */
  fprintf (f, "{\n\
\t\"timestamp\": %ld,\n\
\t\"propertyName\": \"%s\",\n\
\t\"poiName\": \"%s\",\n\
\t\"group\": %ld,\n\
\t\"scanData\": [\n\
",
           time (NULL), info->propertyname, info->poiname, info->group);

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