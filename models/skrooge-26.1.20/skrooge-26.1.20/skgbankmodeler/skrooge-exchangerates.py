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
apikey = sys.argv[2]

url = 'http://api.exchangeratesapi.io/v1/latest?access_key='+apikey+'&base=EUR&symbols='+units[0]+','+units[1]
f = urllib.request.urlopen(url)
print("Date,Price")
data = json.loads(f.read().decode('utf-8'))
print(data['date']+','+str(data['rates'][units[1]]/data['rates'][units[0]]))

