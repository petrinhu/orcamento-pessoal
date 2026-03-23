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
    
    click("Units-1.png")
    sleep(1)
    type(Key.ENTER, KEY_CTRL)
    click("Manual.png")
    paste(Pattern("1305987974969.png").similar(0.82), "unit1")
    paste("5ymbl.png", "u1")
    sleep(1)
    type(Key.ENTER, KEY_CTRL)
    sleep(1)
    type(Key.ENTER, KEY_SHIFT)
    doubleClick(find("1383487091620.png").below().find("unit1u1.png"))
    shared.closeCurrentPage()
    click("AfghanAfghan.png")
    click("Download.png")
    
    wait(2)
    
    click("1383499105796.png")
    
    shared.close()
    pass
except FindFailed:
    shared.generateErrorCapture("unit")
    raise
