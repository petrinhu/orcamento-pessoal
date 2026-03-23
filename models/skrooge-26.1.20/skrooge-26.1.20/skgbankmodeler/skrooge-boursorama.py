#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ***************************************************************************
# * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
# * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
# * SPDX-License-Identifier: GPL-3.0-or-later
# ***************************************************************************
import csv
import enum
import locale
import re
import sys
from collections.abc import Generator, Iterable
from contextlib import contextmanager
from dataclasses import dataclass
from datetime import date, datetime
from typing import TextIO

import bs4
import requests
from requests.models import Response

BASE_URL = "http://www.boursorama.com/bourse/action/graph/ws/download"
LOOKUP_URL = "https://www.boursorama.com/recherche/"
SYMBOL_RE = re.compile(r"/cours/([a-zA-Z0-9]+)")
NAVIGATION_RE = re.compile(r"Page \d+")


class Interval(enum.Enum):
    daily = "1d"
    monthly = "1mo"
    weekly = "1wk"


INTERVAL_TO_PERIOD = {Interval.daily: 1, Interval.monthly: 30, Interval.weekly: 7}

# Always ask for 365 entries
MAX_ITEMS = 365
EXPECTED_DATE_FORMAT = "%d/%m/%Y %H:%M"

session = requests.session()
session.cookies["_pctx"] = "dummy"


@dataclass
class TickerValue:
    date: date
    price: float


def ticker_to_url(ticker: str) -> str:
    response = session.get(LOOKUP_URL, params=dict(query=ticker), allow_redirects=False)
    if not (response.is_permanent_redirect or response.is_redirect):
        raise ValueError(
            "Ticker search did not return a valid redirect: check the symbol"
        )
    redirect = response.headers.get("Location") or ""
    match = SYMBOL_RE.match(redirect)
    if match is None:
        raise ValueError(
            "Ticker search did not return a valid redirect: check the symbol"
        )
    actual_code = match.groups()[0]
    return actual_code


def request_one_page(
    interval: Interval,
    symbol: str,
    start_date: date,
) -> Response:
    params: dict[str, str | int] = {
        "symbol": symbol,
        # Always ask for at most 365 entries
        "length": MAX_ITEMS,
        "period": INTERVAL_TO_PERIOD[interval],
    }
    url = BASE_URL
    response = session.get(url, params=params, stream=True)
    response.encoding = "utf8"
    return response


def download_history(
    interval: Interval, symbol: str, start_date: date
) -> Generator[TickerValue]:
    response = request_one_page(interval, symbol, start_date)
    csv_reader = csv.reader(response.iter_lines(decode_unicode=True), delimiter="\t")
    for idx, line in enumerate(csv_reader):
        # First line is an header
        if idx == 0:
            continue
        if not line:
            continue
        date = datetime.strptime(line[0], EXPECTED_DATE_FORMAT).date()
        value = float(line[1])
        yield TickerValue(date=date, price=value)


def dump_to_csv(symbol: str, ticker_values: Iterable[TickerValue], output: TextIO):
    writer = csv.writer(output)
    writer.writerow(("date", "price"))
    for value in ticker_values:
        writer.writerow((value.date, value.price))


if __name__ == "__main__":
    symbol = sys.argv[1]
    from_date = date.fromisoformat(sys.argv[2])
    interval = Interval(sys.argv[3])

    actual_symbol = ticker_to_url(symbol)
    results = sorted(
        download_history(interval, actual_symbol, from_date),
        key=lambda x: x.date,
        reverse=True,
    )
    dump_to_csv(symbol, results, sys.stdout)
