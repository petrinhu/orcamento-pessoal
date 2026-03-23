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
    
    shared.initAllPlugins()
    
    click("1446153094651.png")
    click("1446153115180.png")
    click("1446153142941.png")
    sleep(1)
    type(Key.ENTER, KEY_CTRL)
    click("1446155010281.png")
    sleep(2)
    type(Key.ENTER, KEY_CTRL)
    click("1446654721017.png")
    
    wait(10)
    
    shared.undo()
    shared.redo()
    
    shared.close()
    pass
except FindFailed:
    shared.generateErrorCapture("budget")
    raise
