#include <unistd.h>
#include <stdio.h>
#include <wiringPiI2C.h>

#define DEVICE_ID 0x08

int
main (int argc, char **argv)
{
  /* Setup I2C communication */
  int fd = wiringPiI2CSetup (DEVICE_ID);
  if (fd == -1)
    {
      printf ("Failed to init I2C communication.\n");
      return -1;
    }
  printf ("I2C communication successfully setup.\n");
  
  int Status = -1;
    while((Status = wiringPiI2CWrite (fd, 'z')) < 0);
}