#!/usr/bin/python3

from numpy import var
from pysnmp.hlapi import *
import requests
import sys
import time

gpsdoHost = sys.argv[1]
influxUrl = 'http://192.168.3.200:8086/write?db=radio_astronomy'

while True:
    # try:
        iterator = getCmd(
            SnmpEngine(),
            CommunityData('status', mpModel=0),
            UdpTransportTarget((gpsdoHost, 161)),
            ContextData(),
            ObjectType(ObjectIdentity('1.3.6.1.2.1.99.1.1.1.4'))
        )

        errorIndication, errorStatus, errorIndex, varBinds = next(iterator)

        if errorIndication:
            print(errorIndication)

        elif errorStatus:
            print('%s at %s' % (errorStatus.prettyPrint(),
                                errorIndex and varBinds[int(errorIndex) - 1][0] or '?'))

        varTuple = varBinds[0]
        proc_temp = int(varTuple[1]) / 1000

        varTuple = varBinds[1]
        dcxo_temp = int(varTuple[1]) / 1000

        body = 'gpsdo,host=%s proc_temp=%.2f,dcxo_temp=%.2f' % (
            gpsdoHost, proc_temp, dcxo_temp
        )
        requests.post(influxUrl, data=body, timeout=1)
    # except:
    #     print('failed to poll or record stats')

    # 1 second update interval
        time.sleep(1)
