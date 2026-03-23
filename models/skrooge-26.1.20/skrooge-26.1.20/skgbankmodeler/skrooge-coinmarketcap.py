#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************

from urllib.request import Request, urlopen
import json
import sys

units=sys.argv[1].split('-')
# url = 'https://pro-api.coinmarketcap.com/v1/cryptocurrency/quotes/historical?symbol='+units[0]+'&convert='+units[1]+'&time_start='+sys.argv[2]+'T00:00:00.000Z&time_end='+sys.argv[3]+'T23:59:59.000Z'
url = 'https://pro-api.coinmarketcap.com/v1/cryptocurrency/quotes/latest?symbol='+units[0]+'&convert='+units[1]

req = Request(url)
req.add_header("X-CMC_PRO_API_KEY", sys.argv[5])
f = urlopen(req)
print("Date,Price")
item=json.loads(f.read().decode('utf-8'))['data'][units[0]]['quote'][units[1]]
print(item["last_updated"][0:10]+','+str(item["price"]))
