#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************

import csv
import requests
import urllib3
import ssl

class CustomHttpAdapter (requests.adapters.HTTPAdapter):
    # "Transport adapter" that allows us to use custom ssl_context.

    def __init__(self, ssl_context=None, **kwargs):
        self.ssl_context = ssl_context
        super().__init__(**kwargs)

    def init_poolmanager(self, connections, maxsize, block=False):
        self.poolmanager = urllib3.poolmanager.PoolManager(
            num_pools=connections, maxsize=maxsize,
            block=block, ssl_context=self.ssl_context)


def get_legacy_session():
    ctx = ssl.create_default_context(ssl.Purpose.SERVER_AUTH)
    ctx.options |= 0x4  # OP_LEGACY_SERVER_CONNECT
    session = requests.session()
    session.mount('https://', CustomHttpAdapter(ctx))
    return session

url = 'https://stats.oecd.org/sdmx-json/data/DP_LIVE/.CPI.TOT.AGRWTH.A/OECD?contentType=csv&detail=code&separator=comma&startPeriod=1980'
resp = get_legacy_session().get(url)
reader = csv.reader(resp.text.split('\n'), delimiter=',')
data = {}
next(reader, None)  # skip the headers
for row in reader:
    if len(row)>0:
        if row[0] not in data:
            data[row[0]] = []
        data[row[0]].append((row[5], row[6], 1))
for attr, value in data.items():
    value.sort(key=lambda x: x[0], reverse=True)
    computed = []
    cumul = 1
    for i, x in enumerate(value):
        computed.append((x[0], cumul))
        cumul *= (1+float(x[1])/100)

    with open('inflation_'+attr.lower()+'.csv', 'w') as fw:
        writer = csv.writer(fw, lineterminator="\n")
        writer.writerow(("Date","Price"))
        for row in computed:
            writer.writerow(row)
    print(attr.lower()+',', end = '')
print()
