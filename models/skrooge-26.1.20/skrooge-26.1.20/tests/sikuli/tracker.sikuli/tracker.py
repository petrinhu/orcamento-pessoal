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
    
    click("1305985754504.png")
    paste(Pattern("1305985800311.png").similar(0.87), "tracker1")
    sleep(1)
    type(Key.ENTER, KEY_CTRL)
    paste("Nanatradar.png", "_b")
    sleep(1)
    type(Key.ENTER, KEY_SHIFT)
    doubleClick("CtracIer1b00.png")
    r=find("Operations-1.png").right()
    c=r.find("1305987216474.png")
    click(c)
    click("1305986344895.png")
    click("ShowOpened.png")
    click("Closed.png")
    
    
    shared.close()
    pass
except FindFailed:
    shared.generateErrorCapture("tracker")
    raise
