#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
# works on all platforms
import os

# get the directory containing your running .sikuli
myPath = os.path.dirname(getBundlePath())
if not myPath in sys.path: sys.path.append(myPath)
import shared
try:
    setAutoWaitTimeout(10)
    
    shared.initSimple()
   
    shared.createAccount("bank", "account1")
    shared.createAccount("bank", "account2")
    click("Operations-2.png")
    click("1306077949040.png")
    paste("Amount.png", "100")
    type(Key.ENTER)
    sleep(1)
    type(Key.ENTER, KEY_CTRL)
    paste("Payee.png", "Payee1")
    paste("Mode.png", "Card")
    paste("Comment.png", "Comment")
    paste("Category.png", "Cat1")
    paste("Traclrer.png", "Track1")
    click("1306077966015.png")
    sleep(1)
    type(Key.ENTER, KEY_SHIFT)
    
    sleep(1)
    show=find("Shcw.png")
    click(show)
    t=find("1383394583085.png")
    click(t)
    sleep(1)
    type(Key.ENTER, KEY_CTRL)
    
    sleep(5)
    show=find("Shcw.png")
    click(show)
    o=find("1383394655825.png")
    click(o)
    
    click("TransFer.png")
    sleep(1)
    type(Key.ENTER, KEY_CTRL)
    
    o=find("CardPayee1Co.png")
    rightClick(o)
    click("Switchhighli.png")
    rightClick(o)
    click("Point.png")
    type("d", KEY_CTRL)
    type("i", KEY_CTRL)
    type("w", KEY_CTRL)
    
    type(Key.DELETE)
    type("a", KEY_CTRL)
    type("g", KEY_CTRL)
    wait(1)
    type("g", KEY_CTRL|KEY_SHIFT)
    wait(1)
    type("m", KEY_CTRL|KEY_SHIFT)
    sleep(1)
    type(Key.ENTER, KEY_SHIFT)
    
    click("1306079549775.png", KEY_CTRL)
    
    sleep(1)
    click(show)
    click("Hideallchecl.png")
    
    sleep(1)
    click(show)
    click("ccunt2.png")
    t=find("1306079716239.png")
    type("a", KEY_CTRL)
    click(t)
    wait(1)
    click(t)
    wait(1)
    click(t)
    
    click("Shares-1.png")
    paste("Amwntfshares.png", "300")
    sleep(1)
    type(Key.ENTER, KEY_CTRL)
    
    shared.openReport()
    
    shared.close()
    pass
except FindFailed:
    shared.generateErrorCapture("operation")
    raise
