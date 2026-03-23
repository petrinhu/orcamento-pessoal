#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************

import urllib.request
import json
import sys
import datetime

units=sys.argv[1].split('-')
apikey = sys.argv[2]

import requests

url = 'https://api.apilayer.com/exchangerates_data/latest?base=EUR&symbols='+units[0]+','+units[1]
payload = {}
headers= {"apikey": apikey}
response = requests.request("GET", url, headers=headers, data = payload)

print("Date,Price")
data = json.loads(response.text)
print(data['date']+','+str(data['rates'][units[1]]/data['rates'][units[0]]))

