#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
import os
import shutil
from sikuli.Sikuli import *

def openFile(fileName):
	print("Opening "+fileName)
	type("o", KEY_CTRL)
	if exists("1446152114435.png"):
		click("1446152575740.png")
	paste(Pattern("1446149387754.png").similar(0.59), fileName)
	click("1446152645440.png")
	
def openAllPages():	
	type("1", KEY_CTRL)
	type("2", KEY_CTRL)
	type("3", KEY_CTRL)
	type("4", KEY_CTRL)
	type("5", KEY_CTRL)
	type("6", KEY_CTRL)
	type("7", KEY_CTRL)
	type("8", KEY_CTRL)
	type("9", KEY_CTRL)
	type("0", KEY_CTRL+KEY_ALT)	
	type("1", KEY_CTRL+KEY_ALT)	
	type("2", KEY_CTRL+KEY_ALT)	
	type("3", KEY_CTRL+KEY_ALT)	
	
def closeCurrentPage():
	type("w", KEY_CTRL)

def initSimple():
	print("Initialisation with "+Env.getSikuliVersion())
	App.open("skrooge  > /dev/null 2>&1")
	sleep(5)
	#click("1446152344094.png")	

def initAllPlugins():
	initSimple()
	openFile(os.getenv('IN')+"all_plugins.skg")
	sleep(5)
	click("1446152986402.png")

def close():
	App.close("skrooge")

def undo():
    type("z", KEY_CTRL)

def redo():
    type("z", KEY_CTRL+KEY_SHIFT)

def openSettings():
	click("Setting.png")
	click("gunfigureSrg.png")
	click("Vox.png")
	
def createAccount(bankName, accountName):
	type("2", KEY_CTRL)
	paste("1383133694348.png", bankName)
	paste("1383133566859.png", accountName)
	sleep(1)
	type(Key.ENTER, KEY_CTRL)

	closeCurrentPage()

def openReport():
	type("r", KEY_META)

def generateErrorCapture(name):
    img=capture(SCREEN)
    errorCapture=os.path.join(os.getenv('OUT'), name+"/error.png")
    shutil.move(img, errorCapture)
    print "Capture at failure:"+errorCapture
