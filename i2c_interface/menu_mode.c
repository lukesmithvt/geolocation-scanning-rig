#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h> // millis
#include <wiringPiI2C.h>

#define DEVICE_ID 0x09
#define WIFI_STR_LEN 40
#define BLE_STR_LEN 7
#define WIFI_COUNT_MAX 50
#define BLE_COUNT_MAX 50
#define BUFFER_SIZE                                                           \
  (WIFI_COUNT_MAX * WIFI_STR_LEN) + (BLE_COUNT_MAX * BLE_STR_LEN)

#define SCAN_REQUEST 0x11
#define READ_WIFI_COUNT 0x12
#define READ_BLE_COUNT 0x13
#define READ_SIGNAL_DATA 0x14
#define TEST_BYTE 0x05

char buf[BUFFER_SIZE];
int wifi_count;
int ble_count;

int
main (int argc, char **argv)
{
  if (argc < 2)
    {
      printf ("Usage: %s <i2c_address>\n",
              argv[0]);
      exit (1);
    }

  /* Setup I2C communication */
  int address = strtol (argv[1], NULL, 16);
  int fd = wiringPiI2CSetupInterface ("/dev/i2c-0", address);
  if (fd == -1)
    {
      printf ("Failed to init I2C communication.\n");
      exit (1);
    }
  printf ("I2C communication successfully setup.\n");

  char cmd;
  int Status = 0;
  int loop = 1;
  while (loop)
    {
      printf ("=== COMMAND MENU ===\n");
      printf ("\ta - Send scan request\n");
      printf ("\tb - Read signal count\n");
      printf ("\tc - Request to read signal data\n");
      printf ("\tz - Send test byte\n");
      printf ("\tq - quit\n");
      scanf (" %c", &cmd);
      printf ("\n");

      switch (cmd)
        {
        case 'a':
          // buf[0] = 'a';
          // printf ("Sending: %c\n", buf[0]);
          // Status = write (fd, buf, 1); // initiate scan
          Status = wiringPiI2CWrite (fd, SCAN_REQUEST);
          printf ("Status = %d\n", Status);
          break;

        case 'b':
          // Status = read (fd, buf, 1); // read number of networks found
          wifi_count = wiringPiI2CRead (fd);
          printf ("Found %d wifi networks.\n", wifi_count);
          ble_count = wiringPiI2CRead (fd);
          printf ("Found %d ble devices.\n", ble_count);
          break;

        case 'c':
          {
            int starttime = millis ();
            Status = read (fd, buf, BUFFER_SIZE);
            // for (int i = 0; i < BUFFER_SIZE; i++)
            // {
            //   buf[i] = wiringPiI2CRead(fd);
            // }
            int delta = millis () - starttime;
            printf ("Read %d bytes:\n", Status);
            for (int i = 0; i < BUFFER_SIZE; i++)
              {
                if (i % 40 == 0)
                  {
                    printf ("\n");
                  }
                printf ("%02X ", buf[i]);
              }
            printf ("\nTime to receive data over I2C is: %d ms\n", delta);
            break;
          }

        case 'z':
          {
            Status = wiringPiI2CWrite (fd, TEST_BYTE);
            printf ("Status = %d\n", Status);
            break;
          }

        case 'q':
          loop = 0;
          break;

        default:
          printf ("Received unknown command: %c.", cmd);
          break;
        }
    }
  return 0;
}
