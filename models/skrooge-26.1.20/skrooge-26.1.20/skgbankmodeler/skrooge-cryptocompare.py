#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************

import urllib.request
import json
import sys
import datetime

units=sys.argv[1].split('-')
mode = sys.argv[4]

nb = min((datetime.datetime.strptime(sys.argv[2], '%Y-%m-%d')-datetime.datetime.strptime(sys.argv[3], '%Y-%m-%d')).days, 2000)
url = 'https://min-api.cryptocompare.com/data/v2/histoday?fsym='+units[0]+'&tsym='+units[1]+'&limit='+str(nb)+'&api_key='+sys.argv[5]
f = urllib.request.urlopen(url)
print("Date,Price")
for item in json.loads(f.read().decode('utf-8'))['Data']['Data']:
    d = datetime.datetime.fromtimestamp(int(item["time"]))
    if mode == '1d' or (mode=='1wk' and d.isoweekday()==1) or (mode=='1mo' and d.day==1):
        print(d.strftime('%Y-%m-%d')+','+str(item['close']))
