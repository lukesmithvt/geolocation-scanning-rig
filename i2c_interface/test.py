import subprocess


print("Scanning now...")
subprocess.run(["/home/pi/i2c/bin/concurrent", "1", "property", "poi", "123"])