#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************

import argparse
import subprocess
import sys
from packaging.version import parse as parse_version

__VERSION__ = "1.0.0"


def main():
    parser = argparse.ArgumentParser(prog='skrooge-woob', description='Skrooge woob Bridge')

    # Global arguments
    parser.add_argument('--version', '-v', action='version', version='%(prog)s ' + __VERSION__, help='Shows version information.')
    parser.add_argument('--output', required=True,  help='The folder to store the csv files.')
    parser.add_argument('--filter', required=True,  help='The account filter. Something like account1|account2|account3')
    parser.add_argument('--woob_option', help='The additional woob options.')


    args = parser.parse_args()
    if args.woob_option == None:
        args.woob_option = ''

    command = ['woob', 'bank', 'ls', args.woob_option, '-q', '-f', 'csv', '-s', 'id,label,balance', '-O', args.output]
    if args.filter != None:
        # Add filter
        command.append('--condition')
        command.append('id|' + ' OR id|'.join(args.filter.split('|')))
    print(command)

    process_result = subprocess.run(command, check=True)
    print(process_result.returncode)
    return process_result.returncode

if __name__ == "__main__":
    sys.exit(main())
