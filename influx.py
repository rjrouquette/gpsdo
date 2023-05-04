#!/usr/bin/python3

import math
from pysnmp.hlapi import *
import requests
import sys
import time


def getSnmpData(address: str, mib: str):
    # issue request
    iterator = getCmd(
        SnmpEngine(),
        CommunityData('status', mpModel=0),
        UdpTransportTarget((address, 161)),
        ContextData(),
        ObjectType(ObjectIdentity(mib))
    )
    # get response
    errorIndication, errorStatus, errorIndex, varBinds = next(iterator)
    if errorIndication or errorStatus:
        return None
    # return data
    return [var[1] for var in varBinds]


def assembleSnmpData(address: str):
    scale = getSnmpData(address, '1.3.6.1.2.1.99.1.1.1.2')
    prec = getSnmpData(address, '1.3.6.1.2.1.99.1.1.1.3')
    value = getSnmpData(address, '1.3.6.1.2.1.99.1.1.1.4')
    names = getSnmpData(address, '1.3.6.1.2.1.99.1.1.1.9')

    result = {}
    for i in range(len(names)):
        result[str(names[i])] = int(value[i]) * math.pow(10, ((int(scale[i]) - 9) * 3) - int(prec[i]))
    return result


gpsdoHost = sys.argv[1]
influxUrl = 'http://192.168.3.200:8086/write?db=radio_astronomy'

while True:
    try:
        data = assembleSnmpData(gpsdoHost)
        fields = ','.join([
            f'{key.replace(".", "_")}={value}' for key, value in data.items()
        ])
        requests.post(influxUrl, data=f'gpsdo,host={gpsdoHost} {fields}', timeout=1)
    except:
        print('failed to poll or record stats')

    # 1 second update interval
    time.sleep(1)
