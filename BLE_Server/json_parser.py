#!/usr/bin/env python3
import json
import time
import os

j_dict = {
    'timestamp': int(time.time()),
    'propertyName': 'Demo Property Test',
    'poiName': '3219 Inside',
    'group': '3',
    'scanData': []
}

path = './raw'
mac_dict = {}
for filename in os.listdir(path):
    filepath = os.path.join(path, filename)
    with open(filepath) as f:
        raw_dict = json.load(f)
        for i in range(len(raw_dict['scanData'])):
            curr_item = raw_dict['scanData'][i]
            if curr_item['mac'] not in mac_dict:
                new_item = {
                    "tech": "",
                    "ssid": "",
                    "mac": "",
                    "count": 0,
                    "rssi_raw": [],
                    "rssi_min": 0,
                    "rssi_max": 0,
                    "rssi_avg": 0
                }
                new_item['tech'] = curr_item['tech']
                new_item['ssid'] = curr_item['ssid']
                new_item['mac'] = curr_item['mac']
                new_item['count'] += curr_item['count']
                new_item['rssi_raw'].append(curr_item['rssi_raw'])
                new_item['rssi_min'] = curr_item['rssi_min']
                new_item['rssi_max'] = curr_item['rssi_max']
                new_item['rssi_avg'] = curr_item['rssi_avg']
                mac_dict[curr_item['mac']] = new_item
            else:
                mac_dict_entry = mac_dict[curr_item['mac']]
                mac_dict_entry['count'] += curr_item['count']
                mac_dict_entry['rssi_raw'].append(curr_item['rssi_raw'])
                mac_dict_entry['rssi_min'] = min(mac_dict_entry['rssi_raw'])
                mac_dict_entry['rssi_max'] = max(mac_dict_entry['rssi_raw'])
                mac_dict_entry['rssi_avg'] = round(sum(mac_dict_entry['rssi_raw']) / len(mac_dict_entry['rssi_raw']))

for mac in mac_dict:
    j_dict['scanData'].append(mac_dict[mac])

with open(str(j_dict['timestamp'])+".json", "w") as outfile:
    json.dump(j_dict, outfile, indent=4)