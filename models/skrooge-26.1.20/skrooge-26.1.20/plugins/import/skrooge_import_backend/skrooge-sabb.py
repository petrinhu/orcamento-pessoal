#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************

"""
Skrooge AqBanking Bridge (SABB)
-------------------------------

Authors:
 * Bernhard Scheirle <bernhard@scheirle.de>

Changelog:
3.0.2 - 2022.07.28
    * Switch from deprecated distutils.version to packaging.version

3.0.1 - 2022.07.28
    * Use ultimateDebitor as payee if present.

3.0.0 - 2022.07.28
    * Requires AqBanking 6.5.0
      Only version 6+ is PSD2 (Payment Services Directiv; Directive (EU) 2015/2366) compliant.
    * Limit transaction download to the last 6 weeks
    * Removed --prefer-valutadate command line option
    * Added some code for optical tans, but does not work yet

2.0.0 - 2019.05.09
    * Added auto repair for certain banks (Sprada, Netbank, Comdirect).
    * Added --disable-auto-repair command line option
    * Added --prefer-valutadate command line option
    * Removed --balance command line option

1.2.0 - 2019.04.28
    * Allow processing of accounts without an IBAN
      (e.g credit card accounts).
      In this case a fake IBAN is used:
      XX00<bank_number><account_number>

1.1.0 - 2018.05.21
    * Added command line parameter --terminal-emulator

1.0.0 - 2017.07.29
    * Initial release

"""

import argparse
import contextlib
import csv
import datetime
import io
import os
import re
import shutil
import subprocess
import sys
import tempfile
from packaging.version import parse as parse_version
from PIL import Image

__VERSION__ = "3.0.2"


class AqDialect(csv.Dialect):
    delimiter = ';'
    quotechar = '"'
    quoting = csv.QUOTE_ALL
    lineterminator = '\n'


class SkroogeDialect(csv.Dialect):
    delimiter = ';'
    quotechar = '"'
    escapechar = '\\'
    quoting = csv.QUOTE_ALL
    lineterminator = '\n'


class AqBankingCli(object):
    Tool = 'aqbanking-cli'
    MinVersion = "6.0.2"

    OpticalTan = "{} opticaltan".format(os.path.abspath(__file__))

    @staticmethod
    def build_command(args):
        com = [AqBankingCli.Tool]
        com.append('--charset=utf-8')
        com.append('--opticaltan="{}"'.format(AqBankingCli.OpticalTan))
        com.extend(args)

        if True:
            print("AqBankingCli.build_command: {}".format(com))

        return com

    @staticmethod
    def check_version():
        process_result = subprocess.run(
            AqBankingCli.build_command(['versions']),
            stdout=subprocess.PIPE
        )
        process_result.check_returncode()
        lines = process_result.stdout.decode("utf-8").splitlines()
        valid_version = False
        for line in lines:
            line = line.strip()
            if line.startswith("AqBanking-CLI:"):
                line = line[14:].strip()
                if parse_version(line) >= parse_version(AqBankingCli.MinVersion):
                    valid_version = True
                    break
        if not valid_version:
            print("Please install a newer version of aqbanking."
                  "At least version {} is required.".format(AqBankingCli.MinVersion))
        return valid_version


class Account(object):
    def __init__(self):
        self.bank_number    = ""
        self.account_number = ""
        self.iban           = ""
        self.fake_iban      = False

    @staticmethod
    def create(iban, bank, account):
        a = Account()
        a.iban = iban
        a.bank_number = bank
        a.account_number = account
        if a.iban == "":
            a.fake_iban = True
            a.iban = "XX00{}{}".format(a.bank_number, a.account_number)
        return a

    def update(self, other):
        if self.bank_number != other.bank_number \
           or self.account_number != other.account_number:
            return False

        if not self.fake_iban or other.fake_iban:
            return True

        self.fake_iban = other.fake_iban
        self.iban = other.iban
        return True

    def toString(self):
        return 'Account({}, {}, {})'.format(self.bank_number,
                                            self.account_number,
                                            self.iban)

    def isValid(self):
        return self.bank_number     != "" \
            and self.account_number != "" \
            and self.iban           != ""

    def nice_iban(self):
        result = ""
        for i, c in enumerate(self.iban):
            if i % 4 == 0:
                result = result + " "
            result = result + c
        return result.strip().upper()


class Accounts(object):
    def __init__(self):
        self._accounts = None

    def _insert(self, account):
        if not account.isValid():
            return

        for a in self._accounts:
            if a.update(account):
                return
        self._accounts.append(account)

    def _initialize(self):
        self._accounts = []

        process_result = subprocess.run(
            AqBankingCli.build_command(['listaccs',
                                        '--template="$(iban)";"$(bankcode)";"$(accountNumber)"']),
            stdout=subprocess.PIPE
        )
        process_result.check_returncode()

        fields = ["iban", "bankcode", "accountNumber"]
        input = process_result.stdout.decode("utf-8")
        data_lines = input.splitlines()[1:]
        reader = csv.DictReader(data_lines, fieldnames=fields, dialect=AqDialect)

        for record in reader:
            account = Account.create(record['iban'],
                                     record['bankcode'],
                                     record['accountNumber'])
            self._insert(account)

    def get_accounts(self):
        if not self._accounts:
            self._initialize()
        return self._accounts


@contextlib.contextmanager
def TemporaryContextFile():
    with tempfile.TemporaryDirectory(prefix="SSAB.") as dir_path:
        context_file_path = os.path.join(dir_path, "context")
        open(context_file_path, 'a').close()
        yield context_file_path


class RepairMan(object):
    def repair_row(self, row):
        pass


class RepairManSpardaNetBank(RepairMan):
    """
    Sparda / Netbank only:
    If the payees name exceeds 27 characters the overflowing characters
    of the name gets stored at the beginning of the purpose field.

    This is the case, when one of the strings listed in Keywords is part
    of the purpose fields but does not start at the beginning.
    In this case, the part leading up to the keyword is to be treated as the
    tail of the payee.
    """

    Keywords = ['SEPA-BASISLASTSCHRIFT',
                'SEPA-ÜBERWEISUNG',
                'SEPA LOHN/GEHALT']

    def repair_row(self, row):
        comment = row['comment']

        for key in self.Keywords:
            offset = comment.find(key)
            if offset >= 0:
                if offset > 0:
                    row['payee'] = row['payee'] + comment[:offset]
                keyEnd = offset + len(key)
                row['mode'] = comment[offset:keyEnd]
                row['comment'] = comment[keyEnd:]
                break
        return row


class RepairManComdirect(RepairMan):
    Keywords = ['WERTPAPIERE',
                'LASTSCHRIFT / BELASTUNG',
                'ÜBERTRAG / ÜBERWEISUNG',
                'KONTOÜBERTRAG',
                'KUPON',
                'SUMME MONATSABRECHNUNG VISA',
                'KONTOABSCHLUSSABSCHLUSS ZINSEN']

    def repair_row(self, row):
        comment = row['comment'].strip()

        for key in self.Keywords:
            if comment.startswith(key):
                row['comment'] = comment[len(key):]
                row['mode'] = key
                break
        return row


class RepairManStrip(RepairMan):
    Keywords = ['date', 'mode', 'comment', 'payee', 'amount']

    def repair_row(self, row):
        for key in self.Keywords:
            row[key] = row[key].strip()
        return row


class RepairManDC(RepairMan):
    Keywords = ['comment']

    def repair_row(self, row):
        for key in self.Keywords:
            row[key] = row[key].replace("\\xdc", "Ü")
        return row


class SABB(object):
    ReturnValue_NormalExit = 0
    ReturnValue_InvalidVersion = 1
    ReturnValue_NoCommandGiven = 10
    ReturnValue_UnknownMimeType = 20
    ReturnValue_UnsupportedMimeType = 21

    def __init__(self):
        self.accounts = Accounts()
        self.repair_mans = [RepairManDC(),
                            RepairManSpardaNetBank(),
                            RepairManComdirect(),
                            RepairManStrip()]

        self.output_folder       = None
        self.terminal_emulator   = None
        self.disable_auto_repair = None

    def get_csv_writer(self, fieldnames, generateHeader=True):
        output = io.StringIO("")
        writer = csv.DictWriter(output, fieldnames=fieldnames, dialect=SkroogeDialect)
        if generateHeader:
            writer.writeheader()
        return output, writer

    def get_accounts(self):
        if not AqBankingCli.check_version():
            return self.ReturnValue_InvalidVersion

        fieldnames_output = ['id']
        output, writer = self.get_csv_writer(fieldnames_output)
        for account in self.accounts.get_accounts():
            row = {}
            row['id'] = account.nice_iban()
            writer.writerow(row)
        print(output.getvalue().strip())
        return self.ReturnValue_NormalExit

    def convert_transactions(self, aqbanking_output, generateHeader):
        fieldnames_input = ['date', 'value', 'remoteName', 'ultimateDebtor', 'purpose']
        reader = csv.DictReader(aqbanking_output.splitlines(), fieldnames=fieldnames_input, dialect=AqDialect)
        fieldnames_output = ['date', 'mode', 'comment', 'payee', 'amount']
        output, writer = self.get_csv_writer(fieldnames_output, generateHeader)
        for record in sorted(reader, key=lambda row: row['date']):
            row = {}
            row['date'] = record['date']
            row['mode'] = ""
            row['comment'] = record['purpose']
            if record['ultimateDebtor'] != "":
                row['payee'] = record['ultimateDebtor'].split('/')[0]
            else:
                row['payee'] = record['remoteName']
            row['amount']  = record['value']
            #  row['unit']    = ""  # record['value_currency']
            if not self.disable_auto_repair:
                for repair_man in self.repair_mans:
                    row = repair_man.repair_row(row)
            writer.writerow(row)
        return output.getvalue()

    def process_context_file(self, context_file_path):
        self.output_folder = os.path.abspath(self.output_folder)
        if not os.path.exists(self.output_folder):
            os.makedirs(self.output_folder)

        files = {}
        for account in self.accounts.get_accounts():
            process_result = subprocess.run(
                AqBankingCli.build_command([
                    'listtrans',
                    '--bank=' + str(account.bank_number),
                    '--account=' + str(account.account_number),
                    '--template="$(dateOrValutaDateAsString)";"$(valueAsString)";"$(remoteName)";"$(ultimateDebtor)";"$(purposeInOneLine)"',
                    '-c',
                    context_file_path]),
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL
            )
            transactions = process_result.stdout.decode("utf-8", errors="backslashreplace")

            output_file_path = os.path.join(self.output_folder, account.nice_iban() + ".csv")
            if output_file_path in files:
                files[output_file_path] = files[output_file_path] + '\n' + self.convert_transactions(transactions, False)
            else:
                files[output_file_path] = self.convert_transactions(transactions, True)

        for path, content in files.items():
            with open(path, 'w') as f:
                f.write(content)

    def download(self):
        if not AqBankingCli.check_version():
            return self.ReturnValue_InvalidVersion

        today = datetime.date.today()
        weeksAgo6 = today + datetime.timedelta(weeks=-6)
        weeksNext = today + datetime.timedelta(weeks=1)
        with TemporaryContextFile() as context_file_path:
            args = ['request',
                    '--ignoreUnsupported',
                    '--transactions',
                    # '--number=1', # Was required as workaround for bug in 6.03
                    '--fromdate={}'.format(weeksAgo6.strftime("%Y%m%d")),
                    '--todate={}'.format(today.strftime("%Y%m%d")),
                    '-c',
                    context_file_path
                    ]

            command = str.split(self.terminal_emulator)
            command.extend(AqBankingCli.build_command(args))
            subprocess.run(command, check=True)

            self.process_context_file(context_file_path)

        return self.ReturnValue_NormalExit


def main():
    parser = argparse.ArgumentParser(prog='sabb', description='Skrooge AqBanking Bridge (SABB)')

    # Global arguments
    parser.add_argument('--version', '-v', action='version', version='%(prog)s ' + __VERSION__, help='Shows version information.')
    subparsers = parser.add_subparsers(title='Commands', dest='command')

    # Command: listaccounts
    parser_listaccounts = subparsers.add_parser('listaccounts', help='Returns a list of accounts that can be queried with AqBanking.')

    # Command: bulkdownload
    parser_download = subparsers.add_parser('bulkdownload',         help='Downloads all transactions of the last 6 weeks into the given output folder')
    parser_download.add_argument('--output_folder', required=True,  help='The folder to store the csv files.')
    parser_download.add_argument(
        '--terminal-emulator',
        required=False,
        default="x-terminal-emulator -e",
        help='The terminal emulator command string that gets used to run the aqbanking user-interactive session. '
        'Use an empty value »""« to not start a new terminal, but reuse the terminal running this command. '
        'Example: "xterm -e". '
        '(Default: "x-terminal-emulator -e")'
    )
    parser_download.add_argument(
        '--disable-auto-repair',
        required=False,
        action='store_true',
        help='Disables bank specific repair steps.'
    )

    # Command: opticaltan
    parser_opticaltan = subparsers.add_parser(
        'opticaltan',
        help='Internal command. Used from {} to display optical tans.'.format(AqBankingCli.Tool))
    parser_opticaltan.add_argument('mime', help="Mime type of the payload.")
    parser_opticaltan.add_argument('payload', help="Mime type dependent payload.")

    args = parser.parse_args()

    sabb = SABB()
    if (args.command == "listaccounts"):
        return sabb.get_accounts()
    elif (args.command == "bulkdownload"):
        sabb.output_folder       = args.output_folder
        sabb.terminal_emulator   = args.terminal_emulator
        sabb.disable_auto_repair = args.disable_auto_repair
        return sabb.download()
        #return sabb.process_context_file("<context file>")
    elif (args.command == "opticaltan"):
        if args.mime == "image/png":
            image = Image.open(args.payload)
            image = image.resize((2 * s for s in image.size))
            image.show()
            return SABB.ReturnValue_NormalExit
        elif args.mime == "text/x-flickercode":
            print("Flicker code is not supported by SABB.")
            print("Configure your AqBanking account to use the manual (non-optical) flicker code.")
            return SABB.ReturnValue_UnsupportedMimeType
        else:
            print("Unsupported MIME type: {}".format(args.mime))
            return SABB.ReturnValue_UnknownMimeType

    parser.print_help()
    return SABB.ReturnValue_NoCommandGiven


if __name__ == "__main__":
    sys.exit(main())
