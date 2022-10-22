#!/usr/bin/python3

"""Copyright (c) 2019, Douglas Otwell

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

import dbus

from advertisement import Advertisement
from service import Application, Service, Characteristic, Descriptor
from gpiozero import CPUTemperature
import subprocess

GATT_CHRC_IFACE = "org.bluez.GattCharacteristic1"
NOTIFY_TIMEOUT = 5000

class ThermometerAdvertisement(Advertisement):
    def __init__(self, index):
        Advertisement.__init__(self, index, "peripheral")
        self.add_local_name("Thermometer")
        self.include_tx_power = True

class ThermometerService(Service):
    THERMOMETER_SVC_UUID = "00000001-710e-4a5b-8d75-3e5b444bc3cf"

    def __init__(self, index):
        self.farenheit = True
        self.scan = False
        self.poi = ''
        self.pname = ""
        self.group = ""

        Service.__init__(self, index, self.THERMOMETER_SVC_UUID, True)
        self.add_characteristic(TempCharacteristic(self))
        self.add_characteristic(UnitCharacteristic(self))
        self.add_characteristic(ScanCharacteristic(self))
        self.add_characteristic(HyperCharacteristic(self))
        self.add_characteristic(Hyper2Characteristic(self))
        self.add_characteristic(Hyper3Characteristic(self))

    def is_farenheit(self):
        return self.farenheit

    def set_farenheit(self, farenheit):
        self.farenheit = farenheit
    
    def is_scan(self):
        return self.scan

    def set_scan(self, scan):
        self.scan = scan
        if self.scan:
            print("Scanning now...")
            subprocess.run(["./concurrent", "1", self.pname, self.poi, self.group])

class TempCharacteristic(Characteristic):
    TEMP_CHARACTERISTIC_UUID = "00000002-710e-4a5b-8d75-3e5b444bc3cf"

    def __init__(self, service):
        self.notifying = False

        Characteristic.__init__(
                self, self.TEMP_CHARACTERISTIC_UUID,
                ["notify", "read"], service)
        self.add_descriptor(TempDescriptor(self))

    def get_temperature(self):
        value = []
        unit = "C"

        cpu = CPUTemperature()
        temp = cpu.temperature
        if self.service.is_farenheit():
            temp = (temp * 1.8) + 32
            unit = "F"

        strtemp = str(round(temp, 1)) + " " + unit
        for c in strtemp:
            value.append(dbus.Byte(c.encode()))

        return value

    def set_temperature_callback(self):
        if self.notifying:
            value = self.get_temperature()
            self.PropertiesChanged(GATT_CHRC_IFACE, {"Value": value}, [])

        return self.notifying

    def StartNotify(self):
        if self.notifying:
            return

        self.notifying = True

        value = self.get_temperature()
        self.PropertiesChanged(GATT_CHRC_IFACE, {"Value": value}, [])
        self.add_timeout(NOTIFY_TIMEOUT, self.set_temperature_callback)

    def StopNotify(self):
        self.notifying = False

    def ReadValue(self, options):
        value = self.get_temperature()

        return value

class TempDescriptor(Descriptor):
    TEMP_DESCRIPTOR_UUID = "2901"
    TEMP_DESCRIPTOR_VALUE = "CPU Temperature"

    def __init__(self, characteristic):
        Descriptor.__init__(
                self, self.TEMP_DESCRIPTOR_UUID,
                ["read"],
                characteristic)

    def ReadValue(self, options):
        value = []
        desc = self.TEMP_DESCRIPTOR_VALUE

        for c in desc:
            value.append(dbus.Byte(c.encode()))

        return value

class UnitCharacteristic(Characteristic):
    UNIT_CHARACTERISTIC_UUID = "00000003-710e-4a5b-8d75-3e5b444bc3cf"

    def __init__(self, service):
        Characteristic.__init__(
                self, self.UNIT_CHARACTERISTIC_UUID,
                ["read", "write"], service)
        self.add_descriptor(UnitDescriptor(self))

    def WriteValue(self, value, options):
        val = str(value[0]).upper()
        if val == "C":
            self.service.set_farenheit(False)
        elif val == "F":
            self.service.set_farenheit(True)

    def ReadValue(self, options):
        value = []

        if self.service.is_farenheit(): val = "F"
        else: val = "C"
        value.append(dbus.Byte(val.encode()))

        return value

class UnitDescriptor(Descriptor):
    UNIT_DESCRIPTOR_UUID = "2901"
    UNIT_DESCRIPTOR_VALUE = "Temperature Units (F or C)"

    def __init__(self, characteristic):
        Descriptor.__init__(
                self, self.UNIT_DESCRIPTOR_UUID,
                ["read"],
                characteristic)

    def ReadValue(self, options):
        value = []
        desc = self.UNIT_DESCRIPTOR_VALUE

        for c in desc:
            value.append(dbus.Byte(c.encode()))

        return value

class ScanCharacteristic(Characteristic):
    SCAN_CHARACTERISTIC_UUID = "00000004-710e-4a5b-8d75-3e5b444bc3cf"

    def __init__(self, service):
        Characteristic.__init__(
                self, self.SCAN_CHARACTERISTIC_UUID,
                ["read", "write"], service)
        self.add_descriptor(UnitDescriptor(self))

    def WriteValue(self, value, options):
        val = str(value[0]).upper()
        if val == "0":
            self.service.set_scan(False)
        elif val == "1":
            self.service.set_scan(True)

    def ReadValue(self, options):
        value = []

        if self.service.is_scan(): val = "1"
        else: val = "0"
        value.append(dbus.Byte(val.encode()))

        return value

class ScanDescriptor(Descriptor):
    SCAN_DESCRIPTOR_UUID = "2902"
    SCAN_DESCRIPTOR_VALUE = "Start the scan"


    def __init__(self, characteristic):
        Descriptor.__init__(
                self, self.SCAN_DESCRIPTOR_UUID,
                ["read"],
                characteristic)

    def ReadValue(self, options):
        value = []
        desc = self.SCAN_DESCRIPTOR_VALUE

        for c in desc:
            value.append(dbus.Byte(c.encode()))

        return value

class HyperCharacteristic(Characteristic):
    POI_CHARACTERISTIC_UUID = "00000005-710e-4a5b-8d75-3e5b444bc3cf"

    def __init__(self, service):
        Characteristic.__init__(
                self, self.POI_CHARACTERISTIC_UUID,
                ["read", "write"], service)
        self.add_descriptor(HyperDescriptor(self))

    def WriteValue(self, value, options):
        res = []
        for i in value:
          res.append(str(i))
        res2 = ''.join(map(str, res))
        self.service.poi = res2

    def ReadValue(self, options):
        poi = self.service.poi
        value = []
        for c in poi:
          value.append(dbus.Byte(c.encode()))

        return value

class HyperDescriptor(Descriptor):
    POI_DESCRIPTOR_UUID = "2903"
    POI_DESCRIPTOR_VALUE = "POI Number"

    def __init__(self, characteristic):
        Descriptor.__init__(
                self, self.POI_DESCRIPTOR_UUID,
                ["read"],
                characteristic)

    def ReadValue(self, options):
        value = []
        desc = self.POI_DESCRIPTOR_VALUE

        for c in desc:
            value.append(dbus.Byte(c.encode()))

        return value

class Hyper2Characteristic(Characteristic):
    PNAME_CHARACTERISTIC_UUID = "00000006-710e-4a5b-8d75-3e5b444bc3cf"

    def __init__(self, service):
        Characteristic.__init__(
                self, self.PNAME_CHARACTERISTIC_UUID,
                ["read", "write"], service)
        self.add_descriptor(Hyper2Descriptor(self))

    def WriteValue(self, value, options):
        res = []
        for i in value:
          res.append(str(i))
        res2 = ''.join(map(str, res))
        self.service.pname = res2

    def ReadValue(self, options):
        poi = self.service.pname
        value = []
        for c in poi:
          value.append(dbus.Byte(c.encode()))

        return value

class Hyper2Descriptor(Descriptor):
    PNAME_DESCRIPTOR_UUID = "2904"
    PNAME_DESCRIPTOR_VALUE = "Property Name"

    def __init__(self, characteristic):
        Descriptor.__init__(
                self, self.PNAME_DESCRIPTOR_UUID,
                ["read"],
                characteristic)

    def ReadValue(self, options):
        value = []
        desc = self.PNAME_DESCRIPTOR_VALUE

        for c in desc:
            value.append(dbus.Byte(c.encode()))

        return value

class Hyper3Characteristic(Characteristic):
    GROUP_CHARACTERISTIC_UUID = "00000007-710e-4a5b-8d75-3e5b444bc3cf"

    def __init__(self, service):
        Characteristic.__init__(
                self, self.GROUP_CHARACTERISTIC_UUID,
                ["read", "write"], service)
        self.add_descriptor(Hyper3Descriptor(self))

    def WriteValue(self, value, options):
        res = []
        for i in value:
          res.append(str(i))
        res2 = ''.join(map(str, res))
        self.service.group = res2

    def ReadValue(self, options):
        poi = self.service.group
        value = []
        for c in poi:
          value.append(dbus.Byte(c.encode()))

        return value

class Hyper3Descriptor(Descriptor):
    GROUP_DESCRIPTOR_UUID = "2905"
    GROUP_DESCRIPTOR_VALUE = "Gropu Number"

    def __init__(self, characteristic):
        Descriptor.__init__(
                self, self.GROUP_DESCRIPTOR_UUID,
                ["read"],
                characteristic)

    def ReadValue(self, options):
        value = []
        desc = self.GROUP_DESCRIPTOR_VALUE

        for c in desc:
            value.append(dbus.Byte(c.encode()))

        return value

app = Application()
app.add_service(ThermometerService(0))
app.register()

adv = ThermometerAdvertisement(0)
adv.register()

try:
    app.run()
except KeyboardInterrupt:
    app.quit()
