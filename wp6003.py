# -*- coding: utf-8 -*-
#
# see. https://www.zukeran.org/shin/d/2020/12/19/co2-sensor-2/
#
import Adafruit_BluefruitLE, uuid
from Adafruit_BluefruitLE.services import UART, DeviceInformation
import struct
import atexit
import time
import datetime
 
DEVICE  = '6003#YOURDEVICEID'	# Your device name
SERVICE = uuid.UUID('0000FFF0-0000-1000-8000-00805F9B34FB')
COMMAND = uuid.UUID('0000FFF1-0000-1000-8000-00805F9B34FB')
SENSOR  = uuid.UUID('0000FFF4-0000-1000-8000-00805F9B34FB')
 
provider = Adafruit_BluefruitLE.get_provider()
provider.initialize()
 
 
def main():
    adapter = provider.get_default_adapter()
    adapter.power_on()
    print('adapter:', adapter.name)
    try:
        print('adapter.start_scan()')
        adapter.start_scan()
        atexit.register(adapter.stop_scan)
        known_uarts = set()
        isnotFound = True
        while(isnotFound):
            found = set(provider.find_devices())
            new = found - known_uarts
            for device in new:
                print('Found device: {0} [{1}]'.format(device.name, device.id))
                if(device.name == DEVICE): 
                    print('connecting...')
                    device.connect()
                    print('connected.')
                    isnotFound = False
                    adapter.stop_scan()
            known_uarts.update(new)
            time.sleep(0.5)
        if device is None:
            raise RuntimeError('Failed to find UART device!')
    finally:
        adapter.stop_scan()
 
    print(device.name)
 
    device.discover([SERVICE],[SENSOR])
    service = device.find_service(SERVICE)
    print('got service',SERVICE)
    command = service.find_characteristic(COMMAND)
    print('got characteristic',COMMAND)
    sensor = service.find_characteristic(SENSOR)
    print('got characteristic',SENSOR)

    print('send initialize command "ee"')
    command.write_value(bytes.fromhex("ee"))
    time.sleep(1)
    print('send initialize command "aa + datetime"')
    t = datetime.datetime.now()
    command.write_value(bytes([0xaa,t.year%100,t.month,t.day,t.hour,t.minute,t.second]))
    time.sleep(1)
    print('send initialize command "ae"')
    command.write_value(bytes.fromhex("ae0105"))
    time.sleep(1)
    print('send initialize command "ab"')
    command.write_value(bytes([0xab]))
 
    def received(data):
        print('Notify:',datetime.datetime.now(),' ',data.hex())
        print("Time: 20",data[1],"/",data[2],"/",data[3]," ",data[4],":",data[5],sep='')
        print('Temp:',(data[6]*256+data[7])/10,'℃')
        print('TVOC:',(data[10]*256+data[11])/1000,'mg/㎥')
        print('HCHO:',(data[12]*256+data[13])/1000,'mg/㎥')
        print('CO2 :',(data[16]*256+data[17]),'ppm')

    sensor.start_notify(received)
    print('start notifications.')
    while (True):
        time.sleep(30)
        command.write_value(bytes([0xab]))
 
provider.run_mainloop_with(main)
